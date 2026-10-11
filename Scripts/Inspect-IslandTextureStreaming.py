"""Read-only loaded-texture inventory for the Island map.

Run with UE 5.8 UnrealEditor-Cmd.exe while no other editor for this project is
open:
  UnrealEditor-Cmd.exe CaptiveSky_2.uproject -ExecutePythonScript="Scripts/Inspect-IslandTextureStreaming.py" -unattended -RenderOffscreen -NoZen

This loads Island and asks the engine to print its current streaming pool size
and loaded texture inventory. It never saves the map or assets. The inventory
is not a camera-specific wanted-mip profile; compare it with `stat streaming`
in the actual view before changing texture settings.
"""

import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
TEXTURE_CANDIDATES = (
    "/Game/PN_FoliageCollection/Textures/FlowerTextures/MasterTextures/Combined_Flowers_A",
    "/Game/PN_FoliageCollection/Textures/grassTextures/MasterTextures/springGrass_Opacity_8k",
    "/Game/PN_interactiveSpruceForest/Textures/Imposter/half_01_imposter_N",
    "/Game/PN_interactiveSpruceForest/Textures/Imposter/half_01_imposter_A",
    "/Game/PN_interactiveSpruceForest/Textures/Imposter/half_01_imposter_O",
    "/Game/PN_interactiveSpruceForest/Textures/MasterTextures/ModelTextures/Leaf_O",
)
MAX_REFERENCER_DEPTH = 10
MAX_REFERENCER_PACKAGES = 5000


def find_island_reference_paths(registry, start_path, reference_options):
    """Return bounded dependency chains from a texture's referencers to Island."""
    queue = [(start_path, [start_path])]
    visited = {start_path}
    matches = []
    cursor = 0
    b_hit_limit = False

    while cursor < len(queue):
        package_name, path = queue[cursor]
        cursor += 1
        if len(path) - 1 >= MAX_REFERENCER_DEPTH:
            continue

        referencers = registry.get_referencers(package_name, reference_options) or []
        for package in referencers:
            parent = str(package)
            if parent == MAP_PATH or parent.startswith(MAP_PATH + "/"):
                matches.append(path + [parent])
                continue
            if parent in visited:
                continue
            if len(visited) >= MAX_REFERENCER_PACKAGES:
                b_hit_limit = True
                break
            visited.add(parent)
            queue.append((parent, path + [parent]))

        if b_hit_limit:
            break

    return matches, len(visited), b_hit_limit


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError("Could not load {} for the read-only texture inventory.".format(MAP_PATH))

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        raise RuntimeError("Island loaded without an editor world; no inventory was requested.")

    unreal.log("[IslandTextureInventory] BEGIN map={} (read-only; no camera-specific wanted-mip claim)".format(MAP_PATH))
    unreal.SystemLibrary.execute_console_command(world, "r.Streaming.PoolSize")
    unreal.SystemLibrary.execute_console_command(world, "ListTextures")

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    reference_options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_game_package_references=True,
        include_editor_only_package_references=True,
        include_searchable_names=False,
        include_soft_management_references=False,
        include_hard_management_references=False,
    )
    for texture_path in TEXTURE_CANDIDATES:
        referencers = registry.get_referencers(texture_path, reference_options) or []
        names = sorted(str(package_name) for package_name in referencers)
        unreal.log("[IslandTextureReferencers] texture={} count={} packages={}".format(
            texture_path, len(names), ",".join(names) if names else "<none>"))
        texture = unreal.EditorAssetLibrary.load_asset(texture_path)
        settings = []
        if texture:
            for property_name in (
                "lod_group", "mip_gen_settings", "max_texture_size", "never_stream",
                "srgb", "compression_settings", "virtual_texture_streaming",
            ):
                try:
                    settings.append("{}={}".format(
                        property_name, texture.get_editor_property(property_name)))
                except Exception as error:
                    settings.append("{}=<unavailable:{}>".format(property_name, error))
        unreal.log("[IslandTextureSettings] texture={} settings={}".format(
            texture_path, ",".join(settings) if settings else "<unavailable>"))
        paths, visited_count, b_hit_limit = find_island_reference_paths(
            registry, texture_path, reference_options)
        examples = [" <- ".join(path) for path in paths[:3]]
        unreal.log("[IslandTextureIslandPaths] texture={} paths={} visited={} capped={} examples={}".format(
            texture_path, len(paths), visited_count, b_hit_limit,
            " || ".join(examples) if examples else "<none>"))
    unreal.log("[IslandTextureInventory] COMPLETE; map and assets were not saved.")


try:
    main()
except Exception:
    unreal.log_error("[IslandTextureInventory] FAILED " + traceback.format_exc())
    raise
