"""Rat body: reshapes the Fab Fox mesh (same skeleton, same skin weights) into a rat and exports an FBX for UE.

    blender -b --factory-startup -P rat.py -- <SK_Fox.fbx> <out_dir>

Only vertex positions and the material slots change, so the Fox skeleton, vertex groups and UVs are kept and every
Fox animation (idle, walk, run, sleep, rest) plays on the rat. Use it at about 0.28x in the engine (a fox is ~107 cm
nose to tail; a rat body is ~25-30 cm). Slot 0 = fur, slot 1 = bare skin (tail, feet, inner ear, nose).
"""
import math
import os
import sys
from collections import defaultdict

import bpy
from mathutils import Vector, noise

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)
for o in list(bpy.data.objects):
    if o.type == "MESH" and not o.name.endswith("LOD0"):       # LOD0 only; UE generates the rest
        bpy.data.objects.remove(o)
mesh_obj = next(o for o in bpy.data.objects if o.type == "MESH")
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
me = mesh_obj.data
names = {g.index: g.name for g in mesh_obj.vertex_groups}


def dom(v):
    return names[max(v.groups, key=lambda g: g.weight).group] if v.groups else ""


orig = [v.co.copy() for v in me.vertices]
new = [c.copy() for c in orig]
dom_of = [dom(v) for v in me.vertices]


def idx(*keys):
    return [i for i, n in enumerate(dom_of) if any(k in n for k in keys)]


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def centroid(ids):
    return sum((orig[i] for i in ids), Vector()) / max(1, len(ids))


# --- tail: long, thin, bare. Slim each segment about its own centre line and stretch it.
tail_ids = idx("Tail")
tail_c = {}
for seg in ("Tail", "Tail1", "Tail2", "Tail3", "Tail4", "Tail5"):
    ids = [i for i in tail_ids if dom_of[i].endswith(" " + seg)]
    if ids:
        tail_c[seg] = (ids, centroid(ids))
base_y = min(c.y for _, c in tail_c.values()) if tail_c else 0.0
for seg, (ids, c) in tail_c.items():
    k = 0.20 if seg in ("Tail", "Tail1") else 0.14
    for i in ids:
        d = orig[i] - c
        out = Vector((d.x * k, d.y, d.z * k))
        new[i] = Vector((c.x + out.x, base_y + (c.y - base_y) * 1.10 + out.y, c.z + out.z))

# --- legs: shorter and slimmer. Legs are compressed toward the hip in z; the skinning stays valid because each
# vertex keeps its bone weights (the legs just swing on a shorter arc). The engine places the mesh by its bounds.
HIP_Z = 29.0
LEG_SHORT = 0.46
for side in ("L", "R"):
    ids = idx("%s Thigh" % side, "%s Calf" % side, "%s HorseLink" % side, "%s Foot" % side,
              "%s UpperArm" % side, "%s Forearm" % side, "%s Hand" % side, "%s Finger" % side)
    if not ids:
        continue
    cx = sum(orig[i].x for i in ids) / len(ids)
    for i in ids:
        c = orig[i]
        z = c.z if c.z > HIP_Z else HIP_Z - (HIP_Z - c.z) * LEG_SHORT
        new[i] = Vector((cx + (c.x - cx) * 0.80, c.y, z))

# --- body: hunched, plump hindquarters, narrower shoulders.
body = idx("Spine", "Spine1", "Pelvis")
ys = [orig[i].y for i in body]
y_lo, y_hi = min(ys), max(ys)
for i in body:
    c = orig[i]
    t = (c.y - y_lo) / max(1e-3, y_hi - y_lo)                    # 0 at the chest end (-Y front) .. 1 at the rump
    hump = 3.0 * math.sin(math.pi * min(1.0, max(0.0, t * 0.9 + 0.05))) * smooth((c.z - 30.0) / 10.0)
    plump = 1.0 + 0.14 * t - 0.06 * (1 - t)
    new[i] = Vector((c.x * plump, c.y, c.z + hump))

# --- head: tapered pointed snout, large round ears.
head = idx("Head")
hz = sum(orig[i].z for i in head) / len(head)
fwd = min(orig[i].y for i in head)                              # the snout points -Y
hy0 = sum(orig[i].y for i in head) / len(head)
ear_pts = [abs(orig[i].x) for i in head if orig[i].z > hz + 4.0]
ear_cx = sum(ear_pts) / len(ear_pts) if ear_pts else 3.0
for i in head:
    c = new[i] if new[i] != orig[i] else orig[i]
    if c.y < hy0 - 2.0:                                         # snout: narrow, lower, longer
        t = smooth((hy0 - 2.0 - c.y) / max(1.0, (hy0 - 2.0 - fwd)))
        sx = 1.0 - 0.45 * t
        sz = 1.0 - 0.40 * t
        c = Vector((c.x * sx, hy0 - 2.0 + (c.y - (hy0 - 2.0)) * 1.18, hz + (c.z - hz) * sz - 0.8 * t))
    if orig[i].z > hz + 4.0:                                    # ears: rounder, wider, a little shorter
        ez = hz + 4.0
        side = 1.0 if orig[i].x > 0 else -1.0
        ecx = side * ear_cx
        c = Vector((ecx + (c.x - ecx) * 1.9, c.y, ez + (min(c.z, ez + 5.0) - ez) * 0.7))
    new[i] = c

for i, v in enumerate(me.vertices):
    v.co = new[i]
me.update()

# --- material slots: 0 fur, 1 bare skin (tail, feet/hands, front of snout, inner ears)
me.materials.clear()
fur = bpy.data.materials.new("M_Rat_Fur")
skin = bpy.data.materials.new("M_Rat_Skin")
me.materials.append(fur)
me.materials.append(skin)
skin_groups = ("Tail", "Foot", "Hand", "Finger")
for p in me.polygons:
    gs = {dom_of[vi] for vi in p.vertices}
    bare = any(any(k in g for k in skin_groups) for g in gs)
    if not bare and any(dom_of[vi].endswith("Head") for vi in p.vertices):
        zs = [new[vi].z for vi in p.vertices]
        ys = [new[vi].y for vi in p.vertices]
        bare = (min(ys) < fwd * 1.0 + 0.9 and True) or (min(zs) > hz + 6.0 and False)   # nose tip
    p.material_index = 1 if bare else 0

# --- preview
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"
sc.render.resolution_x, sc.render.resolution_y = 900, 600
sc.display.shading.light = "STUDIO"
sc.display.shading.color_type = "MATERIAL"
fur.diffuse_color = (0.30, 0.25, 0.21, 1)
skin.diffuse_color = (0.85, 0.62, 0.60, 1)
sc.world = bpy.data.worlds.new("w")
sc.world.color = (0.55, 0.55, 0.58)
cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.type = "ORTHO"
cam.data.ortho_scale = 1.35
ctr = Vector((0, -0.10, 0.28))
for name, d in (("side", Vector((1, 0, 0))), ("three_q", Vector((1, -1, 0.5)).normalized()), ("front", Vector((0, -1, 0.2)).normalized())):
    cam.location = ctr + d * 3
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = os.path.join(OUT, "Preview_%s.png" % name)
    bpy.ops.render.render(write_still=True)

bpy.ops.object.select_all(action="DESELECT")
arm.select_set(True)
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = arm
mesh_obj.name = "SK_Rat_01"
bpy.ops.export_scene.fbx(
    filepath=os.path.join(OUT, "SK_Rat_01.fbx"), use_selection=True, object_types={"ARMATURE", "MESH"},
    global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    axis_forward="-Z", axis_up="Y", bake_space_transform=False,
    add_leaf_bones=False, bake_anim=False, mesh_smooth_type="FACE", use_tspace=True,
    path_mode="COPY", embed_textures=False)
print("RAT_OK verts=%d skinpolys=%d" % (len(me.vertices), sum(1 for p in me.polygons if p.material_index == 1)))
