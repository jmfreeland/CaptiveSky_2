"""Fast, read-only Megaplant metadata inventory.

Run with UE 5.8.3 UnrealEditor-Cmd, -NullRHI and
-ExecutePythonScript=<absolute path>. This uses only the Asset Registry: it
does not load mesh/PVE objects, build Nanite data, open a level, or save assets.
Use Inspect-MegaplantAssets.py for detailed loaded-object inspection only when
the editor/derived-data cost is acceptable.
"""

import json
import traceback

import unreal


ROOT = "/Game/Megaplant_Library"
SPECIES = (
    "Tree_Aleppo_Pine",
    "Tree_Black_Alder",
    "Tree_Common_Hazel",
    "Tree_European_Aspen",
    "Tree_European_Beech",
    "Tree_Norway_Maple",
)
TREE_VARIANT_SUFFIXES = ("_A", "_B", "_C", "_D")


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    registry.scan_paths_synchronous([ROOT], False)

    assets = registry.get_assets_by_path(ROOT, recursive=True) or []
    species_reports = []
    for species in SPECIES:
        prefix = ROOT + "/" + species + "/"
        matching = [item for item in assets if str(item.package_name).startswith(prefix)]
        tree_variants = []
        pve_assets = []
        for item in sorted(matching, key=lambda value: str(value.package_name)):
            name = str(item.asset_name)
            entry = {
                "package": str(item.package_name),
                "asset_name": name,
                "class": str(item.asset_class_path),
            }
            if name.startswith("Tree_") and name.endswith(TREE_VARIANT_SUFFIXES):
                tree_variants.append(entry)
            elif name.startswith("PVE_") or "GrowerPreset" in name or "GrowthData" in name:
                pve_assets.append(entry)
        species_reports.append({
            "species": species,
            "tree_variant_count": len(tree_variants),
            "tree_variants": tree_variants,
            "pve_asset_count": len(pve_assets),
            "pve_assets": pve_assets,
        })

    report = {
        "root": ROOT,
        "asset_registry_entry_count": len(assets),
        "metadata_only": True,
        "species": species_reports,
        "limitations": [
            "Registry class/path metadata does not prove successful object loading, rendering, wind response, or runtime cost.",
            "No mesh or PVE object was loaded; no Nanite assembly or derived mesh data was built by this script.",
        ],
    }
    unreal.log("[MegaplantRegistryAudit] REPORT " + json.dumps(report, sort_keys=True))
    summary = "[MegaplantRegistryAudit] COMPLETE entries={} species={} tree_variants={} pve_assets={} mode=metadata_only"
    unreal.log(summary.format(
        len(assets),
        len(species_reports),
        sum(item["tree_variant_count"] for item in species_reports),
        sum(item["pve_asset_count"] for item in species_reports),
    ))


try:
    main()
except Exception:
    unreal.log_error("[MegaplantRegistryAudit] FAILED " + traceback.format_exc())
    raise
