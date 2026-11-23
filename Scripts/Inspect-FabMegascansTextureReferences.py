"""Read-only trace from the large Fab Megascans textures back to Island."""

import json
import traceback

import unreal


TEXTURE_PATHS = (
    "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/T_xisbchvs_8K_ORM",
    "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs_8K_Displacement",
)
MAP_PATH = "/Game/Maps/Island"
MAX_DEPTH = 16
MAX_PACKAGES = 12000


def find_island_paths(registry, texture_package, options):
    queue = [(texture_package, [texture_package])]
    visited = {texture_package}
    paths = []
    cursor = 0
    capped = False

    while cursor < len(queue):
        package, chain = queue[cursor]
        cursor += 1
        if len(chain) - 1 >= MAX_DEPTH:
            continue
        for parent_id in registry.get_referencers(package, options) or []:
            parent = str(parent_id)
            if parent == MAP_PATH or parent.startswith(MAP_PATH + "/"):
                paths.append(chain + [parent])
                continue
            if parent in visited:
                continue
            if len(visited) >= MAX_PACKAGES:
                capped = True
                break
            visited.add(parent)
            queue.append((parent, chain + [parent]))
        if capped:
            break

    return paths, len(visited), capped


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    registry.scan_paths_synchronous(["/Game"], False)
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_game_package_references=True,
        include_editor_only_package_references=True,
        include_searchable_names=False,
        include_soft_management_references=False,
        include_hard_management_references=False,
    )

    report = {"map": MAP_PATH, "textures": [], "errors": []}
    for path in TEXTURE_PATHS:
        data = unreal.EditorAssetLibrary.find_asset_data(path)
        if not data.is_valid():
            report["errors"].append("Missing asset: " + path)
            continue
        package = str(data.package_name)
        texture = unreal.EditorAssetLibrary.load_asset(path)
        properties = {}
        if texture:
            for name in ("max_texture_size", "mip_gen_settings", "lod_group", "never_stream",
                         "compression_settings", "virtual_texture_streaming"):
                try:
                    properties[name] = str(texture.get_editor_property(name))
                except Exception as error:
                    properties[name] = "unavailable: {}".format(error)
        direct = sorted(str(item) for item in (registry.get_referencers(package, options) or []))
        chains, visited, capped = find_island_paths(registry, package, options)
        report["textures"].append({
            "package": package,
            "class": str(data.asset_class_path),
            "properties": properties,
            "direct_referencers": direct,
            "island_paths": chains,
            "visited_packages": visited,
            "search_capped": capped,
        })

    unreal.log("[FabMegascansTextureReferences] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[FabMegascansTextureReferences] COMPLETE textures={} errors={}".format(
        len(report["textures"]), len(report["errors"])))
    if report["errors"]:
        raise RuntimeError("Fab texture reference audit encountered missing assets: {}".format(report["errors"]))


try:
    main()
except Exception:
    unreal.log_error("[FabMegascansTextureReferences] FAILED " + traceback.format_exc())
    raise
