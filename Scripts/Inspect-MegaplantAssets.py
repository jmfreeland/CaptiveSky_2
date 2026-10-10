"""Read-only inventory of native Fab Megaplant assets in the project.

Run in UE 5.8.3 with -ExecutePythonScript=<absolute path> -unattended
-NoZen -NullRHI -abslog=<absolute log path>. It loads tree A-D variants and
PVE data/presets for class, bounds, LOD, and material-slot inspection. It does
not save assets, change project settings, or load/edit a level. A StaticMesh
class is only a prerequisite for HISM use; this audit does not prove visual,
wind, shading, or runtime-performance compatibility.
"""

import json
import traceback
import unreal


ROOT = "/Game/Megaplant_Library"
SPECIES = (
    "Tree_Common_Hazel",
    "Tree_European_Aspen",
    "Tree_European_Beech",
    "Tree_Norway_Maple",
)


def mesh_details(asset):
    details = {}
    try:
        bounds = asset.get_bounds()
        details["bounds_origin_cm"] = [round(float(v), 2) for v in bounds.origin]
        details["bounds_extent_cm"] = [round(float(v), 2) for v in bounds.box_extent]
    except Exception as error:
        details["bounds_error"] = str(error)
    try:
        details["lod_count"] = int(asset.get_num_lods())
    except Exception as error:
        details["lod_error"] = str(error)
    try:
        property_name = "static_materials" if isinstance(asset, unreal.StaticMesh) else "materials"
        slots = asset.get_editor_property(property_name)
        details["material_slots"] = [
            {
                "name": str(slot.get_editor_property("material_slot_name")),
                "material": (
                    slot.get_editor_property("material_interface").get_path_name()
                    if slot.get_editor_property("material_interface") else None
                ),
            }
            for slot in slots
        ]
    except Exception as error:
        details["material_slots_error"] = str(error)
    return details


def main():
    all_paths = unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False)
    targets = []
    for path in all_paths:
        name = path.rsplit(".", 1)[-1]
        if name.startswith("PVE_") or (name.startswith("Tree_") and name[-2:] in ("_A", "_B", "_C", "_D")):
            targets.append(path)

    report = {"root": ROOT, "species": [], "asset_count": len(all_paths), "errors": []}
    for species in SPECIES:
        entries = []
        prefix = ROOT + "/" + species + "/"
        for path in sorted(targets):
            if not path.startswith(prefix):
                continue
            entry = {"path": path, "errors": []}
            try:
                asset = unreal.load_asset(path)
                if not asset:
                    entry["errors"].append("Asset failed to load")
                else:
                    entry["class"] = asset.get_class().get_name()
                    entry["is_static_mesh"] = isinstance(asset, unreal.StaticMesh)
                    entry["is_skeletal_mesh"] = isinstance(asset, unreal.SkeletalMesh)
                    if entry["is_static_mesh"] or entry["is_skeletal_mesh"]:
                        entry.update(mesh_details(asset))
            except Exception as error:
                entry["errors"].append(str(error))
            entries.append(entry)
        report["species"].append({"name": species, "assets": entries})

    report["tree_variant_count"] = sum(
        1 for species in report["species"] for entry in species["assets"]
        if entry.get("path", "").rsplit(".", 1)[-1].startswith("Tree_")
    )
    report["static_mesh_tree_variants"] = [
        entry["path"] for species in report["species"] for entry in species["assets"]
        if entry.get("is_static_mesh") and entry.get("path", "").rsplit(".", 1)[-1].startswith("Tree_")
    ]
    report["skeletal_mesh_tree_variants"] = [
        entry["path"] for species in report["species"] for entry in species["assets"]
        if entry.get("is_skeletal_mesh") and entry.get("path", "").rsplit(".", 1)[-1].startswith("Tree_")
    ]
    report["limitations"] = [
        "Asset class and metadata do not prove successful render, wind behavior, or runtime performance.",
        "No materials, assets, project settings, or levels are saved or modified.",
        "A tree variant is a HISM candidate only when the loaded asset is a UStaticMesh; material usage still needs testing.",
    ]
    unreal.log("[MegaplantAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[MegaplantAudit] COMPLETE assets={} tree_variants={} static_mesh_trees={} skeletal_mesh_trees={}".format(
        report["asset_count"], report["tree_variant_count"],
        len(report["static_mesh_tree_variants"]), len(report["skeletal_mesh_tree_variants"])))


try:
    main()
except Exception:
    unreal.log_error("[MegaplantAudit] FAILED " + traceback.format_exc())
    raise
