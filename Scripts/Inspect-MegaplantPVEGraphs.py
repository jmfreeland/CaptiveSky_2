"""Read-only inspection of native Megaplant procedural-vegetation assets.

Run with UE 5.8.3 and -EnablePlugins=ProceduralVegetationEditor
-ExecutePythonScript=<absolute path> -unattended -NoZen -NullRHI.
This script verifies installed PVE graphs, growth-data assets, and presets can
load, and reports whether the graph property is exposed to Python. It does not
edit or save assets, project settings, or levels, and does not invoke exporter.
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


def inspect_graph_asset(path):
    entry = {"path": path}
    asset = unreal.load_asset(path)
    if not asset:
        entry["error"] = "Asset failed to load"
        return entry

    entry["class"] = asset.get_class().get_name()
    if entry["class"] == "ProceduralVegetation":
        try:
            asset.get_editor_property("graph")
            entry["graph_access"] = "exposed"
        except Exception as error:
            entry["graph_access"] = (
                "protected" if "protected" in str(error).lower() else "unavailable"
            )
            entry["graph_error"] = str(error)
    return entry


def main():
    all_paths = unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False)
    pve_paths = sorted(
        path for path in all_paths
        if path.rsplit(".", 1)[-1].startswith("PVE_")
        or "GrowerPreset" in path
        or "GrowthData" in path
    )
    report = {"root": ROOT, "asset_count": len(all_paths), "assets": []}
    for species in SPECIES:
        prefix = ROOT + "/" + species + "/"
        for path in pve_paths:
            if path.startswith(prefix):
                try:
                    report["assets"].append(inspect_graph_asset(path))
                except Exception as error:
                    report["assets"].append({"path": path, "error": str(error)})
    unreal.log("[MegaplantPVEAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[MegaplantPVEAudit] COMPLETE assets={} graph_assets={}".format(
        report["asset_count"], len(report["assets"])))


try:
    main()
except Exception:
    unreal.log_error("[MegaplantPVEAudit] FAILED " + traceback.format_exc())
    raise
