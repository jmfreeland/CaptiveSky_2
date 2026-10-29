"""Enable instanced-static-mesh usage on the audited Birch and Hazel roots.

Run in the UE 5.8.3 editor attached to the project's Island. This is deliberately
limited to nine graph-audited ComfyBlender materials. Every target must be clean
and retain its audited two-constant graph before the script changes anything.
All changed roots are recompiled before the first save. Rhododendron is excluded;
its unresolved material-function graph is not safe to repair by toggling usage.
"""

import traceback
import unreal


MATERIAL_PATHS = (
    "/Game/Generated/ComfyBlender/BirchCoppice_Review/Materials/M_Lib_Birch_bark",
    "/Game/Generated/ComfyBlender/BirchCoppice_Review/Materials/M_Lib_Birch_bark_mark",
    "/Game/Generated/ComfyBlender/BirchCoppice_Review/Materials/M_Lib_Birch_leaf_shadow",
    "/Game/Generated/ComfyBlender/BirchCoppice_Review/Materials/M_Lib_Birch_leaf_green",
    "/Game/Generated/ComfyBlender/BirchCoppice_Review/Materials/M_Lib_Birch_leaf_sun",
    "/Game/Generated/ComfyBlender/Codex_WoodlandHazelCoppiceSmoothGroups_20261018/Materials/M_Lib_Coppice_bark",
    "/Game/Generated/ComfyBlender/Codex_WoodlandHazelCoppiceSmoothGroups_20261018/Materials/M_Lib_Coppice_leaf_deep",
    "/Game/Generated/ComfyBlender/Codex_WoodlandHazelCoppiceSmoothGroups_20261018/Materials/M_Lib_Coppice_leaf_mid",
    "/Game/Generated/ComfyBlender/Codex_WoodlandHazelCoppiceSmoothGroups_20261018/Materials/M_Lib_Coppice_leaf_sun",
)

EXPECTED_EXPRESSION_CLASSES = {
    "MaterialExpressionConstant3Vector",
    "MaterialExpressionConstant",
}


def main():
    library = unreal.MaterialEditingLibrary
    assets = unreal.EditorAssetLibrary
    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    materials = []
    dirty_package_paths = {
        package.get_path_name()
        for package in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    }

    # Complete the entire safety preflight before mutating any package.
    for path in MATERIAL_PATHS:
        material = assets.load_asset(path)
        if not isinstance(material, unreal.Material):
            raise RuntimeError("Expected a base Material at {}".format(path))
        package = assets.get_package_for_object(material)
        if package.get_path_name() in dirty_package_paths:
            raise RuntimeError("Refusing to edit dirty asset: {}".format(path))

        expressions = list(library.get_material_expressions(material))
        expression_classes = {expr.get_class().get_name() for expr in expressions}
        if len(expressions) != 2 or expression_classes != EXPECTED_EXPRESSION_CLASSES:
            raise RuntimeError(
                "Graph no longer matches the audited two-constant graph at {}: {}".format(
                    path, sorted(expression_classes)
                )
            )
        materials.append((path, material, bool(library.has_material_usage(material, usage))))

    changed = []
    save_started = False
    try:
        for path, material, was_enabled in materials:
            if was_enabled:
                continue
            library.set_base_material_usage(material, usage, True)
            if not library.has_material_usage(material, usage):
                raise RuntimeError("Usage flag did not enable on {}".format(path))
            changed.append((path, material))
            errors = list(library.recompile_material(material))
            if errors:
                raise RuntimeError("{} failed material compilation: {}".format(path, errors))

        # Saving only begins after all newly requested permutations compile.
        save_started = True
        for path, material in changed:
            if not assets.save_loaded_asset(material):
                raise RuntimeError("Could not save {}".format(path))

        reports = []
        for path, _, was_enabled in materials:
            material = assets.load_asset(path)
            enabled = bool(library.has_material_usage(material, usage))
            if not enabled:
                raise RuntimeError("Post-save usage check failed for {}".format(path))
            reports.append({"path": path, "was_enabled": was_enabled, "enabled_after_save": enabled})

        unreal.log("[GeneratedCoppiceInstancedUsage] COMPLETE " + str(reports))
    except Exception:
        # If compilation fails before saving starts, undo only our in-memory
        # usage changes. Never save a partially compiled set.
        if not save_started:
            for _, material in reversed(changed):
                library.set_base_material_usage(material, usage, False)
                library.recompile_material(material)
        else:
            unreal.log_error(
                "[GeneratedCoppiceInstancedUsage] Save phase failed; inspect the individual packages before retrying."
            )
        unreal.log_error("[GeneratedCoppiceInstancedUsage] FAILED " + traceback.format_exc())
        raise


main()
