"""Enable HISM usage on the three audited Woodland Bluebell materials.

Run in UE 5.8.3 with -ExecutePythonScript=<absolute script path> -unattended
-NullRHI -NoZen -DDC-ForceMemoryCache -abslog=<absolute log path>.
All three materials are preflighted and recompiled before any asset is saved.
Their graph audit found no unresolved material-function references.
"""

import traceback
import unreal


MATERIAL_PATHS = (
    "/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/Materials/M_Lib_Bluebell_bloom",
    "/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/Materials/M_Lib_Bluebell_leaf",
    "/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/Materials/M_Lib_Bluebell_stem",
)


def main():
    library = unreal.MaterialEditingLibrary
    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    pending_saves = []
    reports = []

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

    unreal.log("[BluebellInstancedUsage] COMPLETE " + str(reports))


try:
    main()
except Exception:
    unreal.log_error("[BluebellInstancedUsage] FAILED " + traceback.format_exc())
    raise
