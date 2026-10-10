"""Import one locally generated ComfyBlender FBX and its manifest-driven materials.

Reads Saved/CaptiveSky/ComfyBlender/<ASSET>/ and writes only to
/Game/Generated/ComfyBlender/<ASSET> plus an unreal_import.json report beside the source.
It never opens or saves a level. Existing assets with the same names are replaced in place;
unrelated assets are left alone.

Select the asset with COMFYBLENDER_ASSET, for example:
  $env:COMFYBLENDER_ASSET = 'NoticeBoard'
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=Scripts/Import-ComfyBlenderAsset.py -unattended -NoZen -DDC-ForceMemoryCache
"""

import json
import os
import re
import traceback

import unreal


ASSET_NAME = os.environ.get("COMFYBLENDER_ASSET", "NoticeBoard")
DESTINATION_NAME = os.environ.get("COMFYBLENDER_DESTINATION", ASSET_NAME)
PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_ROOT = os.path.normpath(os.path.join(PROJECT_DIR, "Saved", "CaptiveSky", "ComfyBlender"))
DEST_ROOT = "/Game/Generated/ComfyBlender"


def log(message):
    unreal.log("[ComfyBlenderImport] " + str(message))


def texture_path(asset_dir, reference):
    """Resolve a manifest map reference and ensure it stays inside the generated library."""
    if not isinstance(reference, str):
        return None
    relative = reference.split(" (")[0].strip()
    if not relative.lower().endswith((".png", ".tga", ".jpg", ".jpeg")):
        return None
    full_path = os.path.realpath(os.path.join(asset_dir, relative))
    allowed_root = os.path.realpath(SOURCE_ROOT) + os.sep
    if not full_path.startswith(allowed_root):
        raise ValueError("Manifest texture escapes the ComfyBlender library: {}".format(reference))
    if not os.path.isfile(full_path):
        raise FileNotFoundError(full_path)
    return full_path


def import_files(paths, destination):
    if not paths:
        return []
    tasks = []
    for filename in paths:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", filename)
        task.set_editor_property("destination_path", destination)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("replace_existing_settings", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        imported.extend(str(path) for path in task.get_editor_property("imported_object_paths"))
    return imported


def import_mesh(fbx_path, destination):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    mesh_data = options.get_editor_property("static_mesh_import_data")
    mesh_data.set_editor_property("combine_meshes", True)
    mesh_data.set_editor_property("auto_generate_collision", False)
    # These FBXs are authored in cm; the legacy FBX factory otherwise scales them to metres.
    mesh_data.set_editor_property("import_uniform_scale", 100.0)
    factory = unreal.FbxFactory()
    task.set_editor_property("options", options)
    task.set_editor_property("factory", factory)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = [str(path) for path in task.get_editor_property("imported_object_paths")]
    mesh_paths = [path for path in paths if isinstance(unreal.load_asset(path), unreal.StaticMesh)]
    if not mesh_paths:
        raise RuntimeError("FBX import produced no static mesh: {}".format(paths))
    return mesh_paths[0], paths


def configure_texture(texture, map_name):
    if map_name == "Normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif map_name == "ORM":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    else:
        texture.set_editor_property("srgb", True)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def create_or_reuse_material(package_path, material_name):
    object_path = package_path + "/" + material_name + "." + material_name
    material = unreal.load_asset(object_path)
    if material:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
        return material
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        material_name, package_path, unreal.Material, unreal.MaterialFactoryNew())


def add_expression(material, expression_class, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
    if not expression:
        raise RuntimeError("Could not create material expression {}".format(expression_class))
    return expression


def connect_material(material, expression, output_name, property_name):
    if not unreal.MaterialEditingLibrary.connect_material_property(expression, output_name, property_name):
        raise RuntimeError("Could not connect {} to material property {}".format(expression, property_name))


def create_material(asset_path, material_name, description, textures):
    material_path = asset_path + "/Materials"
    unreal.EditorAssetLibrary.make_directory(material_path)
    material = create_or_reuse_material(material_path, material_name)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("two_sided", False)

    base = description.get("BaseColor")
    if isinstance(base, str):
        sample = add_expression(material, unreal.MaterialExpressionTextureSample, -600, 0)
        sample.set_editor_property("texture", textures["BaseColor"])
        connect_material(material, sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    elif isinstance(base, (list, tuple)) and len(base) >= 3:
        color = add_expression(material, unreal.MaterialExpressionConstant3Vector, -600, 0)
        color.set_editor_property("constant", unreal.LinearColor(float(base[0]), float(base[1]), float(base[2]), 1.0))
        connect_material(material, color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = description.get("Normal")
    if isinstance(normal, str):
        sample = add_expression(material, unreal.MaterialExpressionTextureSample, -600, 220)
        sample.set_editor_property("texture", textures["Normal"])
        connect_material(material, sample, "RGB", unreal.MaterialProperty.MP_NORMAL)

    orm = description.get("ORM")
    if isinstance(orm, str):
        sample = add_expression(material, unreal.MaterialExpressionTextureSample, -600, 440)
        sample.set_editor_property("texture", textures["ORM"])
        for channel, prop in (("R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
                              ("G", unreal.MaterialProperty.MP_ROUGHNESS),
                              ("B", unreal.MaterialProperty.MP_METALLIC)):
            connect_material(material, sample, channel, prop)

    for key, prop, y in (("Roughness", unreal.MaterialProperty.MP_ROUGHNESS, 840),
                         ("Metallic", unreal.MaterialProperty.MP_METALLIC, 1000)):
        if key in description and isinstance(description[key], (int, float)):
            value = add_expression(material, unreal.MaterialExpressionConstant, -300, y)
            value.set_editor_property("r", float(description[key]))
            connect_material(material, value, "", prop)

    emissive = description.get("Emissive")
    if isinstance(emissive, (list, tuple)) and len(emissive) >= 3:
        color = add_expression(material, unreal.MaterialExpressionConstant3Vector, -600, 1220)
        color.set_editor_property("constant", unreal.LinearColor(float(emissive[0]), float(emissive[1]), float(emissive[2]), 1.0))
        strength = float(description.get("EmissiveStrength", 1.0))
        if strength != 1.0:
            scale = add_expression(material, unreal.MaterialExpressionConstant, -600, 1440)
            scale.set_editor_property("r", strength)
            multiply = add_expression(material, unreal.MaterialExpressionMultiply, -300, 1320)
            if not unreal.MaterialEditingLibrary.connect_material_expressions(color, "", multiply, "A"):
                raise RuntimeError("Could not connect emissive color to its strength multiplier")
            if not unreal.MaterialEditingLibrary.connect_material_expressions(scale, "", multiply, "B"):
                raise RuntimeError("Could not connect emissive strength to its multiplier")
            connect_material(material, multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        else:
            connect_material(material, color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def collision_report(mesh):
    try:
        body_setup = mesh.get_editor_property("body_setup")
        aggregate = body_setup.get_editor_property("agg_geom")
        return {
            name: len(aggregate.get_editor_property(name))
            for name in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems")
        }
    except Exception as exc:
        return {"inspection_error": str(exc)}


def verify_import(manifest, mesh, assignments, collisions):
    errors = []
    expected_size = manifest.get("size_cm_xyz")
    if isinstance(expected_size, list) and len(expected_size) == 3:
        bounds = mesh.get_bounding_box()
        actual_size = bounds.max - bounds.min
        actual = [actual_size.x, actual_size.y, actual_size.z]
        for axis, (expected, measured) in enumerate(zip(expected_size, actual)):
            tolerance = max(2.0, abs(float(expected)) * 0.01)
            if abs(float(measured) - float(expected)) > tolerance:
                errors.append("Axis {} scale mismatch: expected {} cm, imported {:.1f} cm (tolerance {:.1f})".format(
                    "XYZ"[axis], expected, measured, tolerance))

    expected_slots = set(manifest.get("material_slots", []))
    assigned_slots = {item["slot"] for item in assignments if item["material"]}
    missing_slots = sorted(expected_slots - assigned_slots)
    if missing_slots:
        errors.append("No imported material assigned for manifest slots: {}".format(", ".join(missing_slots)))

    expected_collisions = manifest.get("collision", [])
    if isinstance(expected_collisions, str):
        expected_collisions = [expected_collisions]
    actual_collision_count = sum(value for key, value in collisions.items() if key.endswith("_elems") and isinstance(value, int))
    if expected_collisions and actual_collision_count < len(expected_collisions):
        errors.append("Collision count mismatch: manifest lists {}, Unreal reports {} shapes".format(
            len(expected_collisions), actual_collision_count))
    if "inspection_error" in collisions:
        errors.append("Collision could not be inspected: {}".format(collisions["inspection_error"]))
    return errors


def run():
    if not re.fullmatch(r"[A-Za-z0-9_]+", ASSET_NAME):
        raise ValueError("COMFYBLENDER_ASSET must be a simple asset folder name")
    if not re.fullmatch(r"[A-Za-z0-9_]+", DESTINATION_NAME):
        raise ValueError("COMFYBLENDER_DESTINATION must be a simple asset folder name")
    asset_dir = os.path.realpath(os.path.join(SOURCE_ROOT, ASSET_NAME))
    if not asset_dir.startswith(os.path.realpath(SOURCE_ROOT) + os.sep):
        raise ValueError("Asset path is outside the ComfyBlender library")
    manifest_path = os.path.join(asset_dir, "manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as handle:
        manifest = json.load(handle)
    fbx_path = os.path.join(asset_dir, manifest["fbx"])
    if not os.path.isfile(fbx_path):
        raise FileNotFoundError(fbx_path)

    texture_sources = {}
    for material_description in manifest.get("materials", {}).values():
        for map_name in ("BaseColor", "Normal", "ORM"):
            resolved = texture_path(asset_dir, material_description.get(map_name))
            if resolved:
                texture_sources[(os.path.basename(resolved), map_name)] = resolved

    asset_path = DEST_ROOT + "/" + DESTINATION_NAME
    texture_folder = asset_path + "/Textures"
    unreal.EditorAssetLibrary.make_directory(texture_folder)
    imported_textures = import_files(sorted(set(texture_sources.values())), texture_folder)
    texture_by_filename = {}
    for obj_path in imported_textures:
        texture = unreal.load_asset(obj_path)
        if isinstance(texture, unreal.Texture2D):
            texture_by_filename[os.path.basename(obj_path.split(".")[0])] = texture

    material_assets = {}
    for material_name, material_description in manifest.get("materials", {}).items():
        textures = {}
        for map_name in ("BaseColor", "Normal", "ORM"):
            source = texture_path(asset_dir, material_description.get(map_name))
            if source:
                texture = texture_by_filename.get(os.path.splitext(os.path.basename(source))[0])
                if not texture:
                    raise RuntimeError("Imported texture missing for {} {}".format(material_name, map_name))
                configure_texture(texture, map_name)
                textures[map_name] = texture
        material_assets[material_name] = create_material(asset_path, material_name, material_description, textures)

    mesh_path, imported_paths = import_mesh(fbx_path, asset_path)
    mesh = unreal.load_asset(mesh_path)
    assignments = []
    slots = mesh.get_editor_property("static_materials")
    for index, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        material = material_assets.get(slot_name)
        if material:
            mesh.set_material(index, material)
        assignments.append({"slot": slot_name, "material": material.get_path_name() if material else None})

    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    size = box.max - box.min
    collisions = collision_report(mesh)
    report = {
        "asset": ASSET_NAME,
        "destination": asset_path,
        "static_mesh": mesh.get_path_name(),
        "imported_objects": imported_paths,
        "size_cm_xyz": [round(size.x, 1), round(size.y, 1), round(size.z, 1)],
        "material_assignments": assignments,
        "collision_shapes": collisions,
        "source_triangles_approx": manifest.get("triangles_approx"),
    }
    report["validation_errors"] = verify_import(manifest, mesh, assignments, collisions)
    report_path = os.path.join(asset_dir, "unreal_import.json")
    with open(report_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
    log(json.dumps(report))
    if report["validation_errors"]:
        raise RuntimeError("Import completed with validation errors: {}".format("; ".join(report["validation_errors"])))


try:
    run()
except Exception:
    unreal.log_error("[ComfyBlenderImport] " + traceback.format_exc())
