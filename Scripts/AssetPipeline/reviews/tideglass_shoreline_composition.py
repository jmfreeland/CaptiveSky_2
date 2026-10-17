"""Render a Blender-only composition study from the local Tideglass plant FBXs.

This is a disposable art-direction aid, not an Unreal material, placement, or
performance approval. Run with Blender 5.2:

  blender.exe -b --factory-startup -P Scripts/AssetPipeline/reviews/tideglass_shoreline_composition.py -- <output-dir>
"""

import json
import math
import os
import sys

import bpy
from mathutils import Vector

PIPE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.abspath(os.path.join(PIPE, "..", ".."))
EXPORT_ROOT = os.path.join(REPO, "Saved", "CaptiveSky", "ComfyBlender")

SPECIES = {
    "samphire": "TideglassRockSamphire",
    "sea_thrift": "TideglassSeaThrift",
    "marram": "TideglassMarramGrass",
    "soft_rush": "TideglassSoftRush",
}

# Locations are metres in Blender space. The groups follow a low, rocky edge,
# a flowering seam, a drier rear edge, and one isolated rush group to test its
# vertical silhouette. They are a composition study, not scatter coordinates.
PLACEMENTS = (
    ("samphire", (-1.72, -0.40, 0.0), 1.48, -14.0),
    ("samphire", (-1.22, -0.06, 0.0), 1.30, 28.0),
    ("samphire", (-0.53, -0.42, 0.0), 1.42, 7.0),
    ("samphire", (0.18, -0.12, 0.0), 1.22, -22.0),
    ("samphire", (0.75, -0.46, 0.0), 1.38, 11.0),
    ("samphire", (1.38, -0.10, 0.0), 1.24, 32.0),
    ("sea_thrift", (-2.28, 0.36, 0.0), 0.82, 12.0),
    ("sea_thrift", (-1.40, 0.78, 0.0), 0.88, -18.0),
    ("sea_thrift", (0.87, 0.71, 0.0), 0.86, 21.0),
    ("sea_thrift", (1.65, 0.25, 0.0), 0.78, -7.0),
    ("marram", (-2.55, 1.34, 0.0), 0.92, -16.0),
    ("marram", (-1.92, 1.82, 0.0), 0.82, 12.0),
    ("marram", (-2.73, 2.10, 0.0), 0.76, 28.0),
    ("soft_rush", (2.12, 0.72, 0.0), 0.90, -8.0),
    ("soft_rush", (2.54, 1.17, 0.0), 0.75, 19.0),
)


def material(name, color, roughness=0.88, metallic=0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    bsdf = next(node for node in mat.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    return mat


def set_candidate_materials(mesh, manifest):
    specs = list(manifest["materials"].values())
    for index, slot in enumerate(mesh.material_slots):
        if index >= len(specs):
            break
        spec = specs[index]
        slot.material = material(
            f"Study_{manifest['asset']}_{index}",
            tuple(spec["BaseColor"]),
            spec.get("Roughness", 0.88),
        )


def create_ground_and_water():
    ground_mat = material("Study_DampCoastalSoil", (0.20, 0.22, 0.15), 0.96)
    water_mat = material("Study_TideglassWater", (0.12, 0.31, 0.36), 0.28, 0.04)
    rock_mat = material("Study_SaltWornStone", (0.31, 0.34, 0.31), 0.93)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.725, -0.16))
    ground = bpy.context.object
    ground.name = "ReviewOnly_CoastalGround"
    ground.dimensions = (7.6, 4.85, 0.32)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    ground.data.materials.append(ground_mat)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -2.42, -0.23))
    water = bpy.context.object
    water.name = "ReviewOnly_WaterPlane"
    water.dimensions = (11.0, 2.0, 0.06)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    water.data.materials.append(water_mat)

    # A low, irregular band of rounded stones makes the wet/dry boundary legible.
    stone_layout = (
        (-3.15, -1.22, 0.31, 0.25, 0.19, -12),
        (-2.27, -1.44, 0.40, 0.29, 0.22, 18),
        (-1.30, -1.17, 0.27, 0.23, 0.15, 31),
        (-0.24, -1.39, 0.34, 0.25, 0.20, -24),
        (0.71, -1.18, 0.42, 0.28, 0.18, 11),
        (1.76, -1.46, 0.30, 0.24, 0.17, -18),
        (2.65, -1.20, 0.39, 0.27, 0.20, 23),
        (3.17, -1.48, 0.24, 0.21, 0.15, -5),
    )
    for index, (x, y, sx, sy, sz, yaw) in enumerate(stone_layout):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1.0, location=(x, y, sz * 0.5))
        stone = bpy.context.object
        stone.name = f"ReviewOnly_ShoreStone_{index + 1:02d}"
        stone.scale = (sx, sy, sz * 0.5)
        stone.rotation_euler[2] = math.radians(yaw)
        stone.data.materials.append(rock_mat)


def load_source_meshes():
    sources = {}
    for species, folder in SPECIES.items():
        asset_dir = os.path.join(EXPORT_ROOT, folder)
        manifest_path = os.path.join(asset_dir, "manifest.json")
        with open(manifest_path, "r", encoding="utf-8") as stream:
            manifest = json.load(stream)
        fbx_path = os.path.join(asset_dir, manifest["fbx"])
        if not os.path.isfile(fbx_path):
            raise FileNotFoundError(f"Missing local candidate FBX: {fbx_path}")
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=fbx_path)
        imported = list(set(bpy.data.objects) - before)
        meshes = [obj for obj in imported if obj.type == "MESH" and not obj.name.startswith(("UCX", "UBX"))]
        for obj in imported:
            if obj not in meshes:
                bpy.data.objects.remove(obj, do_unlink=True)
        if len(meshes) != 1:
            raise RuntimeError(f"Expected one render mesh for {folder}; found {len(meshes)}")
        source = meshes[0]
        source.name = f"Source_{species}"
        set_candidate_materials(source, manifest)
        source.hide_render = True
        source.hide_set(True)
        sources[species] = source
    return sources


def place_instances(sources):
    for index, (species, location, scale, yaw_degrees) in enumerate(PLACEMENTS):
        source = sources[species]
        obj = source.copy()
        obj.data = source.data
        obj.hide_render = False
        obj.hide_set(False)
        bpy.context.collection.objects.link(obj)
        obj.name = f"ReviewOnly_{species}_{index + 1:02d}"
        obj.location = location
        obj.rotation_euler[2] = math.radians(yaw_degrees)
        obj.scale = (scale, scale, scale)
    # The source objects are the first instances and are already positioned.


def setup_lighting_and_camera():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 20
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1440
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.world = bpy.data.worlds.new("ReviewOnly_CoastalSky")
    scene.world.use_nodes = True
    bg = next(node for node in scene.world.node_tree.nodes if node.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (0.32, 0.42, 0.52, 1.0)
    bg.inputs["Strength"].default_value = 0.55

    sun = bpy.data.objects.new("ReviewOnly_SoftAfternoonSun", bpy.data.lights.new("StudySun", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy = 3.2
    sun.data.angle = math.radians(8.0)
    sun.data.color = (1.0, 0.88, 0.72)
    sun.rotation_euler = (math.radians(48), math.radians(6), math.radians(38))

    camera_data = bpy.data.cameras.new("ReviewOnly_Camera")
    camera = bpy.data.objects.new("ReviewOnly_Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = 8.2
    camera.location = (6.3, -8.5, 6.9)
    target = Vector((0.0, -0.05, 0.30))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()


def main():
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) != 1:
        raise SystemExit("Expected exactly one output directory after --")
    output_dir = os.path.abspath(args[0])
    os.makedirs(output_dir, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    create_ground_and_water()
    sources = load_source_meshes()
    place_instances(sources)
    setup_lighting_and_camera()
    output_image = os.path.join(output_dir, "TideglassShorelineComposition.png")
    bpy.context.scene.render.filepath = output_image
    bpy.ops.render.render(write_still=True)
    report = {
        "review_only": True,
        "engine": "Blender Cycles CPU",
        "instances": [
            {"species": species, "location_m": location, "scale": scale, "yaw_deg": yaw}
            for species, location, scale, yaw in PLACEMENTS
        ],
        "output": output_image,
        "unreal_validation": "none; Blender composition study only",
    }
    with open(os.path.join(output_dir, "composition.json"), "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print("TIDEGLASS_COMPOSITION_DONE", json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
