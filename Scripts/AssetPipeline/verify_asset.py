"""Blender headless: re-import the exported FBX, rebuild materials from the exported PNGs only, render.

blender -b --factory-startup -P verify_asset.py -- <Folder>
Writes <Folder>/Preview.png and prints structure facts.
"""
import json
import math
import os
import sys

PIPE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, PIPE)
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import lib  # noqa: E402

folder = sys.argv[sys.argv.index("--") + 1]
D = f"{lib.ROOT}/{folder}"
man = json.load(open(f"{D}/manifest.json"))
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
bpy.ops.import_scene.fbx(filepath=f"{D}/{man['fbx']}")
objs = list(bpy.data.objects)
mesh = next(o for o in objs if o.type == "MESH" and not o.name[:3] in ("UCX", "UBX"))
colls = [o for o in objs if o.name[:3] in ("UCX", "UBX")]
for o in colls:
    o.hide_render = True
print("IMPORTED", mesh.name, "dims", tuple(round(v, 3) for v in mesh.dimensions), "collision", [c.name for c in colls])
print("UV LAYERS", [u.name for u in mesh.data.uv_layers], "SLOTS", [s.material.name for s in mesh.material_slots], "POLYS", len(mesh.data.polygons))
bbox = [mesh.matrix_world @ Vector(c) for c in mesh.bound_box]
zmin = min(v.z for v in bbox)
print("ZMIN", round(zmin, 4))

problems = []
for slot in mesh.material_slots:
    m = slot.material
    name = m.name.split(".")[0]
    spec = man["materials"].get(name)
    if spec is None:
        problems.append(f"no manifest entry for {name}")
        continue
    m.use_nodes = True
    if isinstance(spec.get("BaseColor"), str):
        try:
            lib.textured_material(m, D, spec)
        except Exception as e:  # noqa: BLE001
            problems.append(f"{name}: {e}")
    else:
        b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        b.inputs["Base Color"].default_value = (*spec["BaseColor"], 1)
        b.inputs["Roughness"].default_value = spec.get("Roughness", 0.5)
        b.inputs["Metallic"].default_value = spec.get("Metallic", 0.0)
        if "Emissive" in spec:
            b.inputs["Emission Color"].default_value = (*spec["Emissive"], 1)
            b.inputs["Emission Strength"].default_value = spec.get("EmissiveStrength", 1.0)
print("PROBLEMS", problems)

# stage: ground + sun + sky + camera framed on the bbox
dims = mesh.dimensions
size = max(dims)
center = Vector((sum(v.x for v in bbox) / 8, sum(v.y for v in bbox) / 8, sum(v.z for v in bbox) / 8))
for e in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
    try:
        sc.render.engine = e
        break
    except TypeError:
        pass
sc.render.resolution_x = sc.render.resolution_y = 900
w = bpy.data.worlds.new("W")
sc.world = w
w.use_nodes = True
bg = next(n for n in w.node_tree.nodes if n.type == "BACKGROUND")
bg.inputs["Color"].default_value = (0.50, 0.62, 0.80, 1)
bg.inputs["Strength"].default_value = 0.55
bpy.ops.mesh.primitive_circle_add(vertices=64, radius=max(size * 4, 4), fill_type="NGON", location=(0, 0, 0))
g = bpy.context.active_object
gm = bpy.data.materials.new("G")
gm.use_nodes = True
gb = next(n for n in gm.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
gb.inputs["Base Color"].default_value = (0.20, 0.27, 0.10, 1)
gb.inputs["Roughness"].default_value = 0.95
g.data.materials.append(gm)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sc.collection.objects.link(sun)
sun.data.energy = 3.2
sun.data.color = (1.0, 0.92, 0.80)
sun.data.angle = math.radians(2.5)
sun.rotation_euler = (math.radians(58), math.radians(8), math.radians(52))
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 50
dist = size * 1.75 + 0.35
cam.location = center + Vector((0.9, -1.1, 0.55)).normalized() * dist
cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
sc.render.filepath = f"{D}/Preview.png"
bpy.ops.render.render(write_still=True)
print("VERIFY DONE")
