"""Import the raven and rat bodies built by Scripts/AssetPipeline/creatures/{raven,rat}.py.

Both are imported onto the ORIGINAL pack skeletons, so every Crow / Fox animation plays on them unchanged:
  SK_Raven_01 -> /Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_Skeleton   (material: the Island crow feather material)
  SK_Rat_01   -> /Game/AnimalVarietyPack/Fox/Meshes/SK_Fox_Skeleton     (materials: M_Rat_Fur, M_Rat_Skin built here)
Output: /Game/Characters/Creatures/{Raven,Rat}/. Writes unreal_import.json beside each source pack. Opens no level.

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=Scripts/Import-Creatures.py -unattended -RenderOffscreen -NoZen
(skeletal-mesh FBX import needs render resources, so -NullRHI is not enough)
"""
import json
import os
import traceback

import unreal

SRC = os.environ.get("CREATURE_SRC") or os.path.normpath(os.path.join(unreal.Paths.project_dir(), "Saved", "CaptiveSky", "ComfyBlender", "Creatures"))
DEST = "/Game/Characters/Creatures"
MEL = unreal.MaterialEditingLibrary


def log(m):
    unreal.log("[CreatureImport] " + str(m))


def import_fbx(fbx, dest_dir, skeleton_path):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("create_physics_asset", False)
    ui.set_editor_property("skeleton", unreal.load_asset(skeleton_path))
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    ui.skeletal_mesh_import_data.set_editor_property("update_skeleton_reference_pose", False)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = [str(p) for p in task.get_editor_property("imported_object_paths")]
    meshes = [p for p in paths if "SK_" in p.rsplit("/", 1)[-1]]
    if not meshes:
        raise RuntimeError("no skeletal mesh imported from {}: {}".format(fbx, paths))
    return unreal.load_asset(meshes[0])


def bounds_cm(mesh):
    b = mesh.get_bounds()
    return dict(origin=[b.origin.x, b.origin.y, b.origin.z], extent=[b.box_extent.x, b.box_extent.y, b.box_extent.z])


def flat_material(path, rgb, rough, texture=None):
    name = path.rsplit("/", 1)[-1]
    folder = path.rsplit("/", 1)[0]
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if texture:
        n = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, 0)
        n.set_editor_property("texture", texture)
        MEL.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        n = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
        n.set_editor_property("constant", unreal.LinearColor(*rgb, 1.0))
        MEL.connect_material_property(n, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    m.set_editor_property("used_with_skeletal_mesh", True)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(path)
    return m


def import_texture(png, dest_dir):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.load_asset(dest_dir + "/" + os.path.splitext(os.path.basename(png))[0])
    tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_asset(tex.get_path_name())
    return tex


def set_slots(mesh, materials):
    slots = list(mesh.get_editor_property("materials"))
    if len(slots) != len(materials):
        raise RuntimeError("{} has {} slots, expected {}".format(mesh.get_name(), len(slots), len(materials)))
    for slot, mat in zip(slots, materials):
        slot.set_editor_property("material_interface", mat)
    mesh.set_editor_property("materials", slots)
    if hasattr(mesh, "post_edit_change"):
        mesh.post_edit_change()
    unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())


def main():
    report = {}
    # ---- raven
    mesh = import_fbx(os.path.join(SRC, "Raven", "SK_Raven_01.fbx"), DEST + "/Raven",
                      "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_Skeleton")
    ref = unreal.load_asset("/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow")
    mat = (unreal.load_asset("/Game/AnimalVarietyPack/Crow/Materials/M_Crow_CaptiveSky")
           or unreal.load_asset("/Game/AnimalVarietyPack/Crow/Materials/M_Crow"))
    set_slots(mesh, [mat])
    walk = unreal.load_asset("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_Walk")
    report["raven"] = dict(mesh=mesh.get_path_name(), skeleton=mesh.skeleton.get_path_name(), bounds=bounds_cm(mesh),
                           crow_bounds=bounds_cm(ref), material=mat.get_path_name(),
                           walk_anim_compatible=bool(walk) and walk.get_editor_property("skeleton") == mesh.skeleton)
    # ---- rat
    folder = DEST + "/Rat"
    unreal.EditorAssetLibrary.make_directory(folder + "/Textures")
    fur_tex = import_texture(os.path.join(SRC, "Rat", "Textures", "T_Rat_Fur_BC.png"), folder + "/Textures")
    fur = flat_material(folder + "/M_Rat_Fur", None, 0.85, texture=fur_tex)
    skin = flat_material(folder + "/M_Rat_Skin", (0.55, 0.36, 0.34), 0.55)
    mesh = import_fbx(os.path.join(SRC, "Rat", "SK_Rat_01.fbx"), folder, "/Game/AnimalVarietyPack/Fox/Meshes/SK_Fox_Skeleton")
    set_slots(mesh, [fur, skin])
    walk = unreal.load_asset("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_Walk")
    report["rat"] = dict(mesh=mesh.get_path_name(), skeleton=mesh.skeleton.get_path_name(), bounds=bounds_cm(mesh),
                         fox_bounds=bounds_cm(unreal.load_asset("/Game/AnimalVarietyPack/Fox/Meshes/SK_Fox")),
                         materials=[fur.get_path_name(), skin.get_path_name()],
                         walk_anim_compatible=bool(walk) and walk.get_editor_property("skeleton") == mesh.skeleton)
    out = os.path.join(SRC, "unreal_import.json")
    with open(out, "w") as f:
        json.dump(report, f, indent=2)
    log(json.dumps(report))


try:
    main()
except Exception:
    unreal.log_error("[CreatureImport] FAILED\n" + traceback.format_exc())
