"""Headless check: import the exported FBX, rebuild materials from the exported PNGs only, render.

Run: blender -b --factory-startup -P check_export.py
"""
import json
import math

import bpy

OUT = "D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/CaptiveSky/ComfyBlender/Well"
bpy.ops.wm.open_mainfile(filepath="D:/comfy_projects/textures/well.blend")
sc = bpy.context.scene
man = json.load(open(f"{OUT}/manifest.json"))

# remove the authoring objects, keep camera / sun / ground / world
for o in list(bpy.data.objects):
    if o.type == "MESH" and o.name != "Ground":
        bpy.data.objects.remove(o, do_unlink=True)

before = set(bpy.data.objects.keys())
bpy.ops.import_scene.fbx(filepath=f"{OUT}/SM_Well_01.fbx")
new = [bpy.data.objects[n] for n in bpy.data.objects.keys() if n not in before]
print("IMPORTED", [(o.name, o.type) for o in new])
mesh = next(o for o in new if o.type == "MESH" and not o.name.startswith("UCX_"))
col = [o for o in new if o.name.startswith("UCX_")]
print("COLLISION OBJECTS", [o.name for o in col])
print("DIMS", tuple(round(v, 3) for v in mesh.dimensions), "scale", tuple(mesh.scale), "loc", tuple(round(v, 3) for v in mesh.location))
print("UV LAYERS", [u.name for u in mesh.data.uv_layers], "SLOTS", [s.material.name if s.material else None for s in mesh.material_slots])
print("POLYS", len(mesh.data.polygons))
for o in col:
    o.hide_render = True
    o.hide_viewport = True


def tex(nt, path, cs, loc):
    n = nt.nodes.new("ShaderNodeTexImage")
    n.image = bpy.data.images.load(f"{OUT}/{path}", check_existing=True)
    n.image.colorspace_settings.name = cs
    n.location = loc
    return n


def textured(mat, spec):
    mat.use_nodes = True
    nt = mat.node_tree
    b = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    bc = tex(nt, spec["BaseColor"].split(" ")[0], "sRGB", (-900, 300))
    nrm = tex(nt, spec["Normal"].split(" ")[0], "Non-Color", (-900, -300))
    orm = tex(nt, spec["ORM"].split(" ")[0], "Non-Color", (-900, 0))
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (-600, 0)
    nt.links.new(orm.outputs["Color"], sep.inputs["Color"])
    ao = nt.nodes.new("ShaderNodeMix"); ao.data_type = "RGBA"; ao.blend_type = "MULTIPLY"; ao.location = (-300, 300)
    s = lambda n, nm: next(x for x in n.inputs if x.name == nm and x.enabled)
    s(ao, "Factor").default_value = 1.0
    nt.links.new(bc.outputs["Color"], s(ao, "A"))
    # AO is single channel in R: expand through a combine
    comb = nt.nodes.new("ShaderNodeCombineColor"); comb.location = (-450, 150)
    for i in ("Red", "Green", "Blue"):
        nt.links.new(sep.outputs["Red"], comb.inputs[i])
    nt.links.new(comb.outputs["Color"], s(ao, "B"))
    nt.links.new(next(x for x in ao.outputs if x.name == "Result" and x.enabled), b.inputs["Base Color"])
    nt.links.new(sep.outputs["Green"], b.inputs["Roughness"])
    # DirectX normal -> flip green for Blender
    sn = nt.nodes.new("ShaderNodeSeparateColor"); sn.location = (-650, -300)
    nt.links.new(nrm.outputs["Color"], sn.inputs["Color"])
    inv = nt.nodes.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 1.0; inv.location = (-500, -380)
    nt.links.new(sn.outputs["Green"], inv.inputs[1])
    cn = nt.nodes.new("ShaderNodeCombineColor"); cn.location = (-350, -300)
    nt.links.new(sn.outputs["Red"], cn.inputs["Red"]); nt.links.new(inv.outputs[0], cn.inputs["Green"]); nt.links.new(sn.outputs["Blue"], cn.inputs["Blue"])
    nm = nt.nodes.new("ShaderNodeNormalMap"); nm.location = (-150, -300)
    nt.links.new(cn.outputs["Color"], nm.inputs["Color"]); nt.links.new(nm.outputs["Normal"], b.inputs["Normal"])


def plain(mat, spec):
    mat.use_nodes = True
    b = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (*spec["BaseColor"], 1)
    b.inputs["Roughness"].default_value = spec.get("Roughness", 0.5)
    b.inputs["Metallic"].default_value = spec.get("Metallic", 0.0)


for slot in mesh.material_slots:
    m = slot.material
    name = m.name.split(".")[0]
    spec = man["materials"][name]
    if isinstance(spec["BaseColor"], str):
        textured(m, spec)
    else:
        plain(m, spec)
    print("MAT OK", m.name)

sc.render.filepath = "D:/comfy_projects/textures/outputs/well_check_export.png"
sc.render.resolution_x = sc.render.resolution_y = 900
bpy.ops.render.render(write_still=True)
print("CHECK DONE")
