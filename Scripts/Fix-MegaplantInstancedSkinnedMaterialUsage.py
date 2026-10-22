"""Give the Island's three Megaplant bark instances a project-owned compatible parent.

Run from the Unreal Editor Python menu. The plugin sample material is never modified;
the duplicated parent retains its existing graph and only enables the usage required
by UInstancedSkinnedMeshComponent.
"""

import unreal


SOURCE_PARENT = (
    "/ProceduralVegetationEditor/SampleAssets/Materials/"
    "MasterMaterials/MA_Foliage_Trees"
)
DESTINATION_PARENT = "/Game/Megaplant_Library/Materials/MA_Foliage_Trees_InstancedSkinned"
MATERIAL_INSTANCES = (
    "/Game/Megaplant_Library/Tree_Norway_Maple/Materials/MI_Norway_Maple_Bark_01",
    "/Game/Megaplant_Library/Tree_European_Aspen/Materials/MI_European_Aspen_Bark_01",
    "/Game/Megaplant_Library/Tree_European_Beech/Materials/MI_European_Beech_Bark_01",
)


def _load_required(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Required asset is missing: {}".format(path))
    return asset


source_parent = _load_required(SOURCE_PARENT)
if not isinstance(source_parent, unreal.Material):
    raise RuntimeError("Expected a base Material at {}".format(SOURCE_PARENT))

if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION_PARENT):
    project_parent = _load_required(DESTINATION_PARENT)
else:
    project_parent = unreal.EditorAssetLibrary.duplicate_asset(
        SOURCE_PARENT, DESTINATION_PARENT
    )
    if not project_parent:
        raise RuntimeError("Could not duplicate plugin parent into project content")

if not isinstance(project_parent, unreal.Material):
    raise RuntimeError("Project-owned destination is not a base Material")

usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_SKINNED_MESH
unreal.MaterialEditingLibrary.set_base_material_usage(project_parent, usage, True)

if not unreal.MaterialEditingLibrary.has_material_usage(project_parent, usage):
    raise RuntimeError("InstancedSkinnedMesh usage did not enable on the project parent")

updated_instances = []
for instance_path in MATERIAL_INSTANCES:
    instance = _load_required(instance_path)
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        raise RuntimeError("Expected a MaterialInstanceConstant at {}".format(instance_path))
    if instance.get_editor_property("parent") != project_parent:
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, project_parent)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    if instance.get_editor_property("parent") != project_parent:
        raise RuntimeError("Reparent verification failed for {}".format(instance_path))
    updated_instances.append(instance)

assets_to_save = [project_parent] + updated_instances
for asset in assets_to_save:
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError("Could not save {}".format(asset.get_path_name()))

unreal.log(
    "Megaplant bark material fix saved: {} and {} instances; plugin sample remains untouched.".format(
        project_parent.get_path_name(), len(updated_instances)
    )
)
