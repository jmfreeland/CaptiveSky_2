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
    "sea_pea": "TideglassSeaPea",
}

# Locations are metres relative to a gently curving waterline. Plants are
# arranged as broken habitat patches rather than even bands. These are a
# composition study, not scatter coordinates.
PLACEMENTS = (
    ("samphire", (-2.55, 0.45), 1.32, -14.0),
    ("samphire", (-2.20, 0.76), 1.18, 28.0),
    ("samphire", (-1.18, 0.52), 1.38, 7.0),
    ("samphire", (-0.82, 0.82), 1.20, -22.0),
    ("samphire", (0.18, 0.44), 1.34, 11.0),
    ("samphire", (0.52, 0.77), 1.14, 32.0),
    ("samphire", (1.63, 0.47), 1.30, -9.0),
    ("samphire", (1.96, 0.78), 1.16, 18.0),
    ("sea_thrift", (-2.13, 1.18), 0.83, 12.0),
    ("sea_thrift", (-1.72, 1.54), 0.77, -18.0),
    ("sea_thrift", (0.40, 1.25), 0.88, 21.0),
    ("sea_thrift", (0.83, 1.60), 0.78, -7.0),
    ("sea_thrift", (2.50, 1.30), 0.86, 15.0),
    ("marram", (-2.83, 1.75), 0.92, -16.0),
    ("marram", (-2.35, 2.08), 0.82, 12.0),
    ("marram", (-2.95, 2.42), 0.76, 28.0),
    ("soft_rush", (2.08, 1.16), 0.88, -8.0),
    ("soft_rush", (2.52, 1.54), 0.74, 19.0),
    ("sea_pea", (-0.54, 1.25), 0.88, -18.0),
    ("sea_pea", (0.02, 1.64), 0.82, 11.0),
    ("sea_pea", (1.30, 1.40), 0.92, 26.0),
)


def shoreline_y(x):
    return -1.42 + 0.20 * math.sin(0.92 * x) + 0.07 * math.sin(2.15 * x + 0.6)


def ground_z(x, y):
    distance_from_shore = max(0.0, y - shoreline_y(x))
    return 0.012 + 0.035 * math.sin(1.25 * x + 0.4) + 0.018 * math.sin(2.3 * y - 0.7) \
        + min(0.32, distance_from_shore * 0.065)


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
    sand_mat = material("Study_WetSandAndShells", (0.34, 0.29, 0.21), 0.82)
    water_mat = material("Study_TideglassWater", (0.055, 0.19, 0.22), 0.18, 0.025)
    rock_mat = material("Study_SaltWornStone", (0.30, 0.31, 0.28), 0.91)

    def mottled(mat, colors, scale, bump_strength=0.0):
        nodes = mat.node_tree.nodes
        links = mat.node_tree.links
        shader = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = 3.2
        noise.inputs["Roughness"].default_value = 0.72
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].position = 0.24
        ramp.color_ramp.elements[0].color = (*colors[0], 1.0)
        ramp.color_ramp.elements[1].position = 0.78
        ramp.color_ramp.elements[1].color = (*colors[1], 1.0)
        links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
        links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
        if bump_strength > 0.0:
            bump = nodes.new("ShaderNodeBump")
            bump.inputs["Strength"].default_value = bump_strength
            bump.inputs["Distance"].default_value = 0.035
            links.new(noise.outputs["Fac"], bump.inputs["Height"])
            links.new(bump.outputs["Normal"], shader.inputs["Normal"])

    mottled(ground_mat, ((0.075, 0.080, 0.055), (0.24, 0.23, 0.15)), 1.7, 0.12)
    mottled(sand_mat, ((0.18, 0.16, 0.115), (0.42, 0.34, 0.235)), 3.0, 0.08)
    mottled(water_mat, ((0.025, 0.10, 0.12), (0.09, 0.28, 0.29)), 5.0, 0.07)

    def surface(name, start_y, end_y, height_fn, rows=28):
        columns = 65
        vertices = []
        for row in range(rows):
            t = row / (rows - 1)
            for column in range(columns):
                x = -4.25 + 8.5 * column / (columns - 1)
                y0, y1 = start_y(x), end_y(x)
                y = y0 + (y1 - y0) * t
                vertices.append((x, y, height_fn(x, y)))
        faces = []
        for row in range(rows - 1):
            for column in range(columns - 1):
                lower = row * columns + column
                faces.append((lower, lower + 1, lower + columns + 1, lower + columns))
        mesh = bpy.data.meshes.new(name + "Mesh")
        mesh.from_pydata(vertices, [], faces)
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        obj.data.materials.append(ground_mat if name.endswith("Ground") else sand_mat)
        for polygon in mesh.polygons:
            polygon.use_smooth = True
        return obj

    surface(
        "ReviewOnly_CoastalGround",
        lambda x: shoreline_y(x) - 0.03,
        lambda _x: 3.45,
        ground_z,
        rows=32,
    )
    surface(
        "ReviewOnly_WetSandRibbon",
        lambda x: shoreline_y(x) - 0.015,
        lambda x: shoreline_y(x) + 0.58,
        lambda x, y: ground_z(x, y) + 0.006,
        rows=7,
    )

    # Water follows the irregular bank and sits slightly below the wet edge.
    water_vertices = []
    water_faces = []
    columns, rows = 65, 20
    for row in range(rows):
        t = row / (rows - 1)
        for column in range(columns):
            x = -4.25 + 8.5 * column / (columns - 1)
            edge = shoreline_y(x) - 0.02
            water_vertices.append((x, -4.25 + (edge + 4.25) * t, -0.045))
    for row in range(rows - 1):
        for column in range(columns - 1):
            lower = row * columns + column
            water_faces.append((lower, lower + 1, lower + columns + 1, lower + columns))
    water_mesh = bpy.data.meshes.new("ReviewOnly_WaterMesh")
    water_mesh.from_pydata(water_vertices, [], water_faces)
    water_mesh.update()
    water = bpy.data.objects.new("ReviewOnly_CurvedWaterEdge", water_mesh)
    bpy.context.collection.objects.link(water)
    water.data.materials.append(water_mat)
    for polygon in water_mesh.polygons:
        polygon.use_smooth = True

    # A broken, low stone rhythm follows the bend without forming a hard border.
    stone_layout = (
        (-3.55, -0.03, 0.25, 0.18, 0.14, -12),
        (-2.93, 0.13, 0.36, 0.24, 0.20, 18),
        (-1.83, -0.08, 0.23, 0.18, 0.13, 31),
        (-1.41, 0.18, 0.33, 0.23, 0.18, -24),
        (-0.15, -0.04, 0.42, 0.29, 0.20, 11),
        (0.96, 0.19, 0.28, 0.21, 0.17, -18),
        (1.77, -0.07, 0.38, 0.25, 0.20, 23),
        (2.73, 0.12, 0.27, 0.20, 0.15, -5),
        (3.52, -0.05, 0.34, 0.24, 0.18, 8),
    )
    for index, (x, edge_offset, sx, sy, sz, yaw) in enumerate(stone_layout):
        y = shoreline_y(x) + edge_offset
        z = ground_z(x, y) + sz * 0.42
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0, location=(x, y, z))
        stone = bpy.context.object
        stone.name = f"ReviewOnly_ShoreStone_{index + 1:02d}"
        stone.scale = (sx, sy, sz * 0.5)
        stone.rotation_euler[2] = math.radians(yaw)
        stone.data.materials.append(rock_mat)

    # Small, sparse shell flecks catch the eye at the transition but do not
    # turn the shore into a decorative pebble carpet.
    shell_mat = material("Study_ShellFleck", (0.52, 0.43, 0.31), 0.78)
    for index, (x, offset, size, yaw) in enumerate((
        (-2.43, 0.34, 0.055, 20), (-0.51, 0.28, 0.045, -32),
        (1.28, 0.37, 0.060, 8), (3.05, 0.31, 0.048, 44),
    )):
        y = shoreline_y(x) + offset
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=1.0,
                                             location=(x, y, ground_z(x, y) + 0.018))
        shell = bpy.context.object
        shell.name = f"ReviewOnly_ShellFleck_{index + 1:02d}"
        shell.scale = (size, size * 0.62, size * 0.22)
        shell.rotation_euler[2] = math.radians(yaw)
        shell.data.materials.append(shell_mat)
        for polygon in shell.data.polygons:
            polygon.use_smooth = True


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
    for index, (species, (x, shore_offset), scale, yaw_degrees) in enumerate(PLACEMENTS):
        source = sources[species]
        obj = source.copy()
        obj.data = source.data
        obj.hide_render = False
        obj.hide_set(False)
        bpy.context.collection.objects.link(obj)
        obj.name = f"ReviewOnly_{species}_{index + 1:02d}"
        y = shoreline_y(x) + shore_offset
        obj.location = (x, y, ground_z(x, y) + 0.004)
        obj.rotation_euler[2] = math.radians(yaw_degrees)
        obj.scale = (scale, scale, scale)
    # Source mesh objects remain hidden; only these review copies are rendered.


def setup_lighting_and_camera():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 20
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1100
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.world = bpy.data.worlds.new("ReviewOnly_CoastalSky")
    scene.world.use_nodes = True
    bg = next(node for node in scene.world.node_tree.nodes if node.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (0.32, 0.42, 0.52, 1.0)
    bg.inputs["Strength"].default_value = 0.68

    sun = bpy.data.objects.new("ReviewOnly_SoftAfternoonSun", bpy.data.lights.new("StudySun", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy = 3.2
    sun.data.angle = math.radians(8.0)
    sun.data.color = (1.0, 0.88, 0.72)
    sun.rotation_euler = (math.radians(48), math.radians(6), math.radians(38))

    fill = bpy.data.objects.new("ReviewOnly_CoolSkyFill", bpy.data.lights.new("StudyFill", "AREA"))
    scene.collection.objects.link(fill)
    fill.data.energy = 420.0
    fill.data.shape = "DISK"
    fill.data.size = 6.0
    fill.data.color = (0.70, 0.82, 1.0)
    fill.location = (-3.5, -1.0, 4.0)
    fill.rotation_euler = (math.radians(18), 0.0, math.radians(-35))

    camera_data = bpy.data.cameras.new("ReviewOnly_Camera")
    camera = bpy.data.objects.new("ReviewOnly_Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.type = "PERSP"
    camera_data.lens = 35.0
    camera.location = (5.0, -6.6, 2.9)
    target = Vector((0.0, 0.05, 0.57))
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
            {
                "species": species,
                "location_m": [x, shoreline_y(x) + shore_offset,
                               ground_z(x, shoreline_y(x) + shore_offset) + 0.004],
                "scale": scale,
                "yaw_deg": yaw,
            }
            for species, (x, shore_offset), scale, yaw in PLACEMENTS
        ],
        "output": output_image,
        "unreal_validation": "none; Blender composition study only",
    }
    with open(os.path.join(output_dir, "composition.json"), "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print("TIDEGLASS_COMPOSITION_DONE", json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
