"""Render a disposable Tideglass 4 m tidepool composition study in Blender 5.2.

This is an offline art-direction preview. It does not create Unreal assets or
claim runtime/material/performance validation. Run:

  blender.exe -b --factory-startup -P Scripts/AssetPipeline/reviews/tidepool_micro_scene.py -- <output-dir>
"""

import json
import math
import os
import sys

import bpy
from mathutils import Vector


REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
CRAB_DIR = os.path.join(REPO, "Saved", "CaptiveSky", "ComfyBlender", "TidepoolCrab")
PATCH_SIZE_M = 4.0
CAMERA_HEIGHT_M = 0.62
RENDER_SIZE = (1400, 980)


def mat(name, color, roughness=0.75, metallic=0.0, transmission=0.0):
    material = bpy.data.materials.new(name)
    material.diffuse_color = (*color, 1.0)
    bsdf = next(n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Transmission Weight"].default_value = transmission
    return material


def noise_material(material, colors, scale, bump=0.0):
    nodes, links = material.node_tree.nodes, material.node_tree.links
    shader = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = scale
    noise.inputs["Detail"].default_value = 4.0
    noise.inputs["Roughness"].default_value = 0.72
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.23
    ramp.color_ramp.elements[0].color = (*colors[0], 1.0)
    ramp.color_ramp.elements[1].position = 0.77
    ramp.color_ramp.elements[1].color = (*colors[1], 1.0)
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    if bump:
        node = nodes.new("ShaderNodeBump")
        node.inputs["Strength"].default_value = bump
        node.inputs["Distance"].default_value = 0.035
        links.new(noise.outputs["Fac"], node.inputs["Height"])
        links.new(node.outputs["Normal"], shader.inputs["Normal"])


def ellipsoid(name, location, scale, material, rotation=(0.0, 0.0, 0.0), segments=24):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    obj.rotation_euler = rotation
    obj.data.materials.append(material)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return obj


def make_shell(name, location, scale, shell_mat):
    # Low-relief coiled shell, scaled to shore-snail rather than beach-conch size.
    points = []
    for index in range(65):
        t = index / 64.0
        angle = t * math.pi * 5.2
        radius = 0.004 + 0.028 * t
        points.append((radius * math.cos(angle), radius * math.sin(angle), t * 0.022))
    curve = bpy.data.curves.new(name + "Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 2
    curve.bevel_depth = 0.0042
    curve.bevel_resolution = 3
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for point, co in zip(spline.points, points):
        point.co = (*co, 1.0)
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.scale = scale
    obj.data.materials.append(shell_mat)
    return obj


def build_terrain():
    mats = {
        "mud": mat("Study_DarkSiltySand", (0.12, 0.115, 0.075), 0.96),
        "wetrock": mat("Study_WetBasalt", (0.18, 0.205, 0.19), 0.31),
        "rock": mat("Study_SaltWornStone", (0.30, 0.31, 0.28), 0.78),
        "pebble": mat("Study_ShellLimestone", (0.49, 0.43, 0.32), 0.64),
        "algae": mat("Study_BrownGreenAlgae", (0.075, 0.13, 0.07), 0.43),
        "shell": mat("Study_PerwinkleShell", (0.24, 0.16, 0.10), 0.38),
        "water": mat("Study_ClearShallowWater", (0.48, 0.68, 0.64), 0.08, transmission=0.12),
    }
    noise_material(mats["mud"], ((0.06, 0.065, 0.045), (0.19, 0.16, 0.105)), 5.0, 0.11)
    noise_material(mats["wetrock"], ((0.08, 0.105, 0.10), (0.25, 0.27, 0.24)), 3.8, 0.10)
    noise_material(mats["rock"], ((0.18, 0.19, 0.17), (0.40, 0.39, 0.33)), 2.6, 0.06)
    water_nodes = mats["water"].node_tree.nodes
    water_links = mats["water"].node_tree.links
    water_bsdf = next(n for n in water_nodes if n.type == "BSDF_PRINCIPLED")
    water_output = next(n for n in water_nodes if n.type == "OUTPUT_MATERIAL")
    transparent = water_nodes.new("ShaderNodeBsdfTransparent")
    surface_mix = water_nodes.new("ShaderNodeMixShader")
    surface_mix.inputs[0].default_value = 0.36
    water_links.new(transparent.outputs["BSDF"], surface_mix.inputs[1])
    water_links.new(water_bsdf.outputs["BSDF"], surface_mix.inputs[2])
    for link in list(water_links):
        if link.to_node == water_output and link.to_socket == water_output.inputs["Surface"]:
            water_links.remove(link)
    water_links.new(surface_mix.outputs[0], water_output.inputs["Surface"])

    # A small continuous seabed keeps the water readable instead of hiding it
    # behind a uniformly opaque pool surface.
    bpy.ops.mesh.primitive_plane_add(size=18.0, location=(0, 0, -0.12))
    floor = bpy.context.object
    floor.name = "ReviewOnly_4m_SiltyPoolFloor"
    floor.data.materials.append(mats["mud"])

    # Irregular shallow water surface. A radial grid avoids the raised lip and
    # artificial bowl silhouette produced by scaling a sphere.
    rings, segments = 12, 96
    water_vertices = [(0.10, -0.28, 0.028)]
    for ring in range(1, rings + 1):
        radius = ring / rings
        for segment in range(segments):
            angle = 2.0 * math.pi * segment / segments
            edge = 1.0 + 0.045 * math.sin(3.0 * angle + 0.7) + 0.026 * math.sin(7.0 * angle - 0.3)
            x = 0.10 + 1.42 * radius * edge * math.cos(angle)
            y = -0.28 + 0.88 * radius * edge * math.sin(angle)
            z = 0.028 + 0.004 * math.sin(4.0 * x + 1.7 * y) * radius
            water_vertices.append((x, y, z))
    water_faces = []
    for segment in range(segments):
        water_faces.append((0, 1 + segment, 1 + (segment + 1) % segments))
    for ring in range(1, rings):
        inner = 1 + (ring - 1) * segments
        outer = inner + segments
        for segment in range(segments):
            nxt = (segment + 1) % segments
            water_faces.append((inner + segment, outer + segment, outer + nxt, inner + nxt))
    water_mesh = bpy.data.meshes.new("ReviewOnly_ClearShallowPoolMesh")
    water_mesh.from_pydata(water_vertices, [], water_faces)
    water_mesh.update()
    water = bpy.data.objects.new("ReviewOnly_ClearShallowPool", water_mesh)
    bpy.context.collection.objects.link(water)
    water.data.materials.append(mats["water"])
    for polygon in water_mesh.polygons:
        polygon.use_smooth = True

    # Fractured basalt shelves create a broken, walkable edge; avoid a neat ring.
    plates = (
        (-1.45, -0.32, (0.65, 0.42, 0.105), (-0.05, 0.18, -0.38)),
        (-0.62, 0.63, (0.68, 0.37, 0.13), (0.12, -0.08, 0.22)),
        (0.72, 0.72, (0.75, 0.40, 0.14), (-0.11, 0.14, -0.27)),
        (1.55, -0.27, (0.56, 0.34, 0.105), (0.06, -0.17, 0.55)),
        (-1.57, -1.12, (0.46, 0.30, 0.075), (0.04, 0.20, 0.08)),
        (1.57, -1.17, (0.42, 0.26, 0.075), (-0.10, -0.13, -0.21)),
        (-0.94, -1.06, (0.48, 0.29, 0.075), (0.04, 0.20, 0.08)),
        (0.02, 1.53, (0.84, 0.34, 0.13), (0.07, 0.11, 0.14)),
    )
    for i, (x, y, scale, rotation) in enumerate(plates):
        z = -0.005 + (i % 3) * 0.025
        ellipsoid(f"ReviewOnly_BasaltShelf_{i + 1:02d}", (x, y, z), scale, mats["wetrock"], rotation)

    # A few dry backslope stones and pebbles break the edge scale.
    for i, (x, y, size) in enumerate((
        (-1.73, 0.95, 0.16), (-1.55, 1.25, 0.11), (1.64, 1.12, 0.18),
        (-1.62, -1.35, 0.12), (1.67, -0.82, 0.13), (0.02, 1.73, 0.10),
    )):
        ellipsoid(f"ReviewOnly_Pebble_{i + 1:02d}", (x, y, 0.015), (size * 1.25, size, size * 0.48), mats["pebble"])

    # Submerged gravel and shell fragments give shallow water a visible scale cue.
    underwater = (
        (-0.82, -0.28, 0.13), (-0.35, -0.62, 0.09), (0.05, -0.19, 0.11),
        (0.52, -0.38, 0.08), (0.89, -0.05, 0.12), (0.34, 0.28, 0.07),
        (-0.18, 0.38, 0.085),
    )
    for i, (x, y, size) in enumerate(underwater):
        ellipsoid(f"ReviewOnly_SubmergedGravel_{i + 1:02d}", (x, y, -0.082),
                  (size, size * 0.78, size * 0.30), mats["pebble"] if i % 2 else mats["rock"])

    # Sparse low algae strips stay on the wet rock surfaces, leaving open sightlines.
    for i, (x, y, angle, length) in enumerate((
        (-1.35, -0.25, 0.2, 0.20), (-0.63, 0.62, -0.5, 0.18),
        (0.78, 0.73, 0.8, 0.22), (1.48, -0.20, -0.2, 0.16),
    )):
        plate_z = (0.10, 0.12, 0.13, 0.10)[i]
        ellipsoid(f"ReviewOnly_AlgaePatch_{i + 1:02d}", (x, y, plate_z), (length, 0.065, 0.009), mats["algae"], (0.0, 0.0, angle))
    return mats


def import_crab():
    manifest_path = os.path.join(CRAB_DIR, "manifest.json")
    with open(manifest_path, encoding="utf-8") as stream:
        manifest = json.load(stream)
    fbx = os.path.join(CRAB_DIR, manifest["fbx"])
    if not os.path.isfile(fbx):
        raise FileNotFoundError(f"Missing authored crab source: {fbx}")
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=fbx)
    imported = [obj for obj in bpy.data.objects if obj not in before]
    meshes = [obj for obj in imported if obj.type == "MESH" and not obj.name.startswith(("UCX", "UBX"))]
    for obj in imported:
        if obj not in meshes:
            bpy.data.objects.remove(obj, do_unlink=True)
    if len(meshes) != 1:
        raise RuntimeError(f"Expected one crab mesh, found {len(meshes)}")
    crab = meshes[0]
    crab.name = "ReviewOnly_ExistingTidepoolCrab"
    crab.location = (-0.94, -1.05, 0.105)
    crab.rotation_euler[2] = math.radians(22.0)
    crab.scale = (0.37, 0.37, 0.37)
    for index, slot in enumerate(crab.material_slots):
        specs = list(manifest["materials"].values())
        if index < len(specs):
            spec = specs[index]
            slot.material = mat(f"Study_CrabMaterial_{index}", tuple(spec["BaseColor"]), spec.get("Roughness", 0.75))
    return crab


def import_coastal_plants():
    plants = (
        ("TideglassSeaThrift", "SM_TideglassSeaThrift_01.fbx", (-1.55, 1.43), 0.66, 18.0),
        ("Codex_TideglassGlasswort_20261017/TideglassGlasswort", "SM_TideglassGlasswort_01.fbx", (1.47, 1.33), 0.76, -24.0),
    )
    for plant_index, (folder, fallback_fbx, (x, y), scale, yaw) in enumerate(plants):
        asset_dir = os.path.join(REPO, "Saved", "CaptiveSky", "ComfyBlender", folder)
        manifest_path = os.path.join(asset_dir, "manifest.json")
        with open(manifest_path, encoding="utf-8") as stream:
            manifest = json.load(stream)
        fbx = os.path.join(asset_dir, manifest.get("fbx", fallback_fbx))
        if not os.path.isfile(fbx):
            raise FileNotFoundError(f"Missing coastal plant source: {fbx}")
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=fbx)
        imported = [obj for obj in bpy.data.objects if obj not in before]
        meshes = [obj for obj in imported if obj.type == "MESH" and not obj.name.startswith(("UCX", "UBX"))]
        for obj in imported:
            if obj not in meshes:
                bpy.data.objects.remove(obj, do_unlink=True)
        if len(meshes) != 1:
            raise RuntimeError(f"Expected one mesh for {folder}; found {len(meshes)}")
        plant = meshes[0]
        plant.name = f"ReviewOnly_CoastalPlant_{plant_index + 1:02d}"
        plant.location = (x, y, 0.015)
        plant.rotation_euler[2] = math.radians(yaw)
        plant.scale = (scale, scale, scale)
        for index, slot in enumerate(plant.material_slots):
            specs = list(manifest["materials"].values())
            if index < len(specs):
                spec = specs[index]
                slot.material = mat(f"Study_Plant_{plant_index}_{index}", tuple(spec["BaseColor"]), spec.get("Roughness", 0.85))


def add_shells_and_ripple(mats):
    for i, (x, y, scale, yaw) in enumerate((
        (-0.82, 0.98, (0.88, 0.88, 0.88), 14),
        (0.92, 0.68, (0.76, 0.76, 0.76), -28),
        (1.28, -0.75, (0.72, 0.72, 0.72), 42),
    )):
        shell = make_shell(f"ReviewOnly_SnailShell_{i + 1:02d}", (x, y, 0.14), scale, mats["shell"])
        shell.rotation_euler[2] = math.radians(yaw)
    # One understated annular ripple makes the crab's waterline legible.
    bpy.ops.mesh.primitive_torus_add(major_radius=0.18, minor_radius=0.006, major_segments=48, minor_segments=8,
                                     location=(0.02, -0.48, 0.055))
    ripple = bpy.context.object
    ripple.name = "ReviewOnly_SingleSurfaceRipple"
    ripple.data.materials.append(mat("Study_RippleHighlight", (0.46, 0.66, 0.65), 0.22, metallic=0.05))


def setup_camera_and_lights():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 20
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = RENDER_SIZE
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.view_transform = "AgX"
    world = bpy.data.worlds.new("ReviewOnly_ClearCoastalSky")
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.34, 0.48, 0.57, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.55
    scene.world = world

    sun = bpy.data.objects.new("ReviewOnly_SoftCoastalSun", bpy.data.lights.new("StudySun", "SUN"))
    scene.collection.objects.link(sun)
    sun.data.energy = 2.4
    sun.data.angle = math.radians(12)
    sun.data.color = (1.0, 0.88, 0.73)
    sun.rotation_euler = (math.radians(35), math.radians(-14), math.radians(28))

    fill = bpy.data.objects.new("ReviewOnly_SkyFill", bpy.data.lights.new("StudyFill", "AREA"))
    scene.collection.objects.link(fill)
    fill.data.energy = 210.0
    fill.data.shape = "DISK"
    fill.data.size = 3.2
    fill.data.color = (0.72, 0.84, 1.0)
    fill.location = (-2.0, -2.3, 2.8)
    fill.rotation_euler = (math.radians(22), 0.0, math.radians(-32))

    camera_data = bpy.data.cameras.new("ReviewOnly_LowTidepoolCamera")
    camera = bpy.data.objects.new("ReviewOnly_LowTidepoolCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera_data.lens = 35.0
    camera.location = (0.20, -3.20, CAMERA_HEIGHT_M)
    target = Vector((0.0, -0.58, 0.08))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()


def main():
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 1:
        raise SystemExit("Expected one output directory after --")
    output = os.path.abspath(args[0])
    os.makedirs(output, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mats = build_terrain()
    import_crab()
    import_coastal_plants()
    add_shells_and_ripple(mats)
    setup_camera_and_lights()
    scene = bpy.context.scene
    scene.render.filepath = os.path.join(output, "TidepoolMicroScene.png")
    bpy.ops.render.render(write_still=True)
    report = {
        "review_only": True,
        "patch_size_m": PATCH_SIZE_M,
        "camera_height_m": CAMERA_HEIGHT_M,
        "crab_source": os.path.relpath(os.path.join(CRAB_DIR, "SM_TidepoolCrab_01.fbx"), REPO),
        "render_engine": "Blender Cycles CPU",
        "output": scene.render.filepath,
        "unreal_validation": "none; offline composition study only",
    }
    with open(os.path.join(output, "composition.json"), "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print("TIDEPOOL_MICRO_SCENE_DONE", json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
