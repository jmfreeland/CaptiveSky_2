"""Enable HISM usage on plant base materials whose graphs passed the reference audit.

Run in UE 5.8.3 with -ExecutePythonScript=<absolute script path> -unattended
-NullRHI -NoZen -DDC-ForceMemoryCache -abslog=<absolute log path>.
The three targets are preflighted and recompiled before any asset is saved. The
Rhododendron master is intentionally excluded because its graph still has four
unresolved material-function references; see docs/findings/2026-10-04-plant-material-dependencies.md.
"""

import traceback
import unreal


MATERIAL_PATHS = (
    "/Game/Plants/Materials/Festuca_gautieri_LD/MM_Festuca_gautieri_LD",
    "/Game/Plants/Materials/Phalaris_arundinacea_LD/MM_Phalaris_arundinacea_LD",
    "/Game/Plants/Materials/Typha_latifolia_LD/MM_Typha_latifolia_LD",
)


def main():
    library = unreal.MaterialEditingLibrary
    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    pending_saves = []
    reports = []

    # Preflight all assets and compile every required permutation before saving any.
    for path in MATERIAL_PATHS:
        material = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(material, unreal.Material):
            raise RuntimeError("Expected a base Material at {}".format(path))

        was_enabled = bool(library.has_material_usage(material, usage))
        if not was_enabled:
            library.set_base_material_usage(material, usage, True)
            if not library.has_material_usage(material, usage):
                raise RuntimeError("Usage flag did not enable on {}".format(path))
            errors = list(library.recompile_material(material))
            if errors:
                raise RuntimeError("{} failed material compilation: {}".format(path, errors))
            pending_saves.append((path, material))
        reports.append({"path": path, "was_enabled": was_enabled})

    for path, material in pending_saves:
        if not unreal.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError("Could not save {}".format(path))

    for report in reports:
        material = unreal.EditorAssetLibrary.load_asset(report["path"])
        enabled = bool(library.has_material_usage(material, usage))
        if not enabled:
            raise RuntimeError("Post-save usage check failed for {}".format(report["path"]))
        report["enabled_after_save"] = enabled

    unreal.log("[PlantInstancedUsage] COMPLETE " + str(reports))


try:
    main()
except Exception:
    unreal.log_error("[PlantInstancedUsage] FAILED " + traceback.format_exc())
    raise
