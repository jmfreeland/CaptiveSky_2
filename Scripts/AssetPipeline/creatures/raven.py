"""Raven body: reshapes the Fab Crow (same skeleton, same skin weights) into a raven and exports an FBX for UE.

    blender -b --factory-startup -P raven.py -- <SK_Crow.fbx> <out_dir>

A raven differs from a crow in silhouette: a deep, heavy bill with an arched culmen, a shaggy throat, a longer
wedge-shaped tail (not a rounded fan), a heavier chest/neck and thicker legs. Only vertex positions change, so
the bone hierarchy, vertex groups and UVs are the crow's and every crow animation plays unchanged.
"""
import math
import os
import sys

import bpy
from mathutils import Vector, noise

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)
mesh_obj = next(o for o in bpy.data.objects if o.type == "MESH")
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
me = mesh_obj.data
names = {g.index: g.name for g in mesh_obj.vertex_groups}
n_verts = len(me.vertices)


def dom(v):
    return names[max(v.groups, key=lambda g: g.weight).group] if v.groups else ""


def has(v, *keys):
    n = dom(v)
    return any(k in n for k in keys)


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


orig = [v.co.copy() for v in me.vertices]
new = [c.copy() for c in orig]

# --- bill: forward of y=-15.2 on the head. Deeper and wider at the base, longer, arched, slightly hooked.
BILL_Y0, BILL_TIP = -15.2, -19.5
bill = [i for i, v in enumerate(me.vertices) if has(v, "Head") and orig[i].y < BILL_Y0]
if bill:
    zs = [orig[i].z for i in bill]
    zc = sum(zs) / len(zs)
    for i in bill:
        c = orig[i]
        t = smooth((BILL_Y0 - c.y) / (BILL_Y0 - BILL_TIP))         # 0 at the base, 1 at the tip
        k = 1.38 - 0.28 * t                                       # depth multiplier: heavy base, blunt tip
        y = BILL_Y0 + (c.y - BILL_Y0) * 1.14                      # longer
        z = zc + (c.z - zc) * k
        x = c.x * (1.30 - 0.15 * t)
        if c.z > zc:
            z += 0.30 * math.sin(math.pi * min(1.0, t * 1.1))      # gentle arched culmen
        z -= 0.35 * t * t                                         # slight droop at the tip
        new[i] = Vector((x, y, z))

# --- shaggy throat: push the underside of head/neck out along the normal with a ragged falloff.
me.calc_normals_split() if hasattr(me, "calc_normals_split") else None
head_bone = arm.data.bones["CROW_ Head"].head_local
for i, v in enumerate(me.vertices):
    if not has(v, "Head", "Neck"):
        continue
    c = orig[i]
    under = (head_bone.z - 1.0) - c.z
    if under > -0.5 and c.y < -6.0 and c.y > -17.0 and new[i] == orig[i]:
        w = smooth((under + 0.5) / 4.5) * smooth((c.y + 17.0) / 4.0)
        jag = 0.5 + 0.5 * noise.noise(Vector((c.x * 0.7, c.y * 0.7, c.z * 0.7)))
        n = v.normal
        new[i] = c + Vector((n.x * 0.55, n.y * 0.4, -abs(n.z) * 0.9 - 0.35)) * w * (0.4 + 0.9 * jag)

# --- tail: lengthen the centre feathers into a wedge and tighten the lateral spread.
ROOT_Y = 7.0
tail = [i for i, v in enumerate(me.vertices) if has(v, "TailLeft", "TailRight")]
for i in tail:
    c = orig[i]
    lat = min(1.0, abs(c.x) / 13.8)
    lengthen = 1.04 + 0.30 * (1.0 - lat) ** 1.2                    # centre longest -> wedge
    y = ROOT_Y + (c.y - ROOT_Y) * lengthen
    x = c.x * 0.78
    new[i] = Vector((x, y, c.z))

# --- heavier raven proportions: deeper chest/neck, a bigger head, thicker legs.
def scale_about(idxs, centre, sx, sy, sz):
    for i in idxs:
        d = new[i] - centre
        new[i] = centre + Vector((d.x * sx, d.y * sy, d.z * sz))

body = [i for i, v in enumerate(me.vertices) if has(v, "Spine", "Pelvis") and new[i] == orig[i]]
scale_about(body, Vector((0, 2.0, 12.0)), 1.10, 1.03, 1.10)
neck = [i for i, v in enumerate(me.vertices) if has(v, "Neck") and new[i] == orig[i]]
scale_about(neck, Vector((0, -8.5, 17.5)), 1.18, 1.0, 1.10)
headv = [i for i, v in enumerate(me.vertices) if has(v, "Head", "Queue") and new[i] == orig[i] and orig[i].y >= BILL_Y0]
scale_about(headv, Vector((0, -12.0, 22.5)), 1.12, 1.08, 1.12)
for side in ("L", "R"):
    leg = [i for i, v in enumerate(me.vertices) if any(("CROW_ %s %s" % (side, p)) in dom(v) for p in ("Thigh", "Calf", "HorseLink"))]
    for i in leg:
        c = new[i]
        ax = Vector((3.5 if side == "L" else -3.5, 0.2, c.z))
        new[i] = ax + (c - ax) * Vector((1.25, 1.25, 1.0))

for i, v in enumerate(me.vertices):
    v.co = new[i]
me.update()
assert len(me.vertices) == n_verts

# --- material slot name for the importer (UE keeps slot order); give it a recognisable name.
if me.materials:
    me.materials[0].name = "M_Raven"

# --- preview renders (workbench) so the reshape is judged before UE ever sees it
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"
sc.render.resolution_x, sc.render.resolution_y = 900, 600
sc.display.shading.light = "STUDIO"
sc.display.shading.color_type = "SINGLE"
sc.display.shading.single_color = (0.12, 0.12, 0.14)
sc.world = bpy.data.worlds.new("w")
sc.world.color = (0.55, 0.55, 0.58)
cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.type = "ORTHO"
cam.data.ortho_scale = 1.05 * 1.1
ctr = Vector((0, 0.0, 0.17))
for name, d in (("side", Vector((1, 0, 0))), ("three_q", Vector((1, -1, 0.5)).normalized()), ("top", Vector((0, 0, 1)))):
    cam.location = ctr + d * 3
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    if name == "top":
        cam.rotation_euler = (0, 0, math.radians(90))
    sc.render.filepath = os.path.join(OUT, "Preview_%s.png" % name)
    bpy.ops.render.render(write_still=True)

# --- export (skeletal mesh + armature, cm, Z-up, same settings the crow came out with)
bpy.ops.object.select_all(action="DESELECT")
arm.select_set(True)
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = arm
mesh_obj.name = "SK_Raven_01"
bpy.ops.export_scene.fbx(
    filepath=os.path.join(OUT, "SK_Raven_01.fbx"), use_selection=True, object_types={"ARMATURE", "MESH"},
    global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    axis_forward="-Z", axis_up="Y", bake_space_transform=False,
    add_leaf_bones=False, bake_anim=False, mesh_smooth_type="FACE", use_tspace=True,
    path_mode="COPY", embed_textures=False)
print("RAVEN_OK bill=%d tail=%d" % (len(bill), len(tail)))
