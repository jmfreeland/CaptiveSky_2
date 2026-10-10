"""Headless: bake the Well's stone/moss shader to an atlas, merge to one mesh, export FBX.

Run: blender -b --factory-startup -P bake_well.py
"""
import math
import os

import bpy

SRC = "D:/comfy_projects/textures/well.blend"
OUT = "D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/CaptiveSky/ComfyBlender/Well"
WORK = "D:/comfy_projects/textures/bake"
SIZE = 4096

os.makedirs(WORK, exist_ok=True)
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=SRC)
sc = bpy.context.scene
O = bpy.data.objects


def select_only(objs, active=None):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = active or objs[0]


# ---------------------------------------------------------------- stone merge
rocks = [O[n] for n in ("Rock_1", "Rock_2", "Rock_3")]
for r in rocks:  # per-block random attribute the shader reads; rocks get a mid value
    if "blk" not in r.data.color_attributes:
        a = r.data.color_attributes.new("blk", "FLOAT_COLOR", "CORNER")
        a.data.foreach_set("color", [0.5, 0.5, 0.5, 1.0] * len(a.data))
blocks = O["Well_Blocks"]
# bmesh-created UV layers come out named "Float2" in Blender 5.2; the shader (and the join with the
# rocks) expects "UVMap". Normalise first or the blocks sample a constant texel.
for ob in [blocks] + rocks:
    ob.data.uv_layers[0].name = "UVMap"
select_only([blocks] + rocks, blocks)
bpy.ops.object.join()
stone = blocks
stone.name = "Well_Stone"
sme = stone.data
assert [u.name for u in sme.uv_layers] == ["UVMap"], [u.name for u in sme.uv_layers]
print("stone polys", len(sme.polygons))

# the shader must sample its tiled textures from the ORIGINAL uv map, not the bake atlas
stone_mat = bpy.data.materials["Mat_Limestone"]
nt = stone_mat.node_tree
tc = next(n for n in nt.nodes if n.type == "TEX_COORD")
uvn = nt.nodes.new("ShaderNodeUVMap")
uvn.uv_map = "UVMap"
for l in list(tc.outputs["UV"].links):
    nt.links.new(uvn.outputs["UV"], l.to_socket)

atlas = sme.uv_layers.new(name="Atlas")
sme.uv_layers.active = atlas
select_only([stone])
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.0025, scale_to_bounds=False)
bpy.ops.uv.pack_islands(margin=0.0025)
bpy.ops.object.mode_set(mode="OBJECT")
sme.uv_layers.active = sme.uv_layers["Atlas"]
sme.uv_layers["Atlas"].active_render = True

# ---------------------------------------------------------------- bake
sc.render.engine = "CYCLES"
cy = sc.cycles
cy.samples = 16
try:
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for dev_type in ("OPTIX", "CUDA"):
        try:
            prefs.compute_device_type = dev_type
            prefs.get_devices()
            gpus = [d for d in prefs.devices if d.type == dev_type]
            if gpus:
                for d in prefs.devices:
                    d.use = d.type == dev_type
                cy.device = "GPU"
                print("bake device", dev_type, [d.name for d in gpus])
                break
        except Exception as e:  # noqa: BLE001
            print("device", dev_type, e)
except Exception as e:  # noqa: BLE001
    print("gpu setup failed, CPU", e)
sc.world.light_settings.distance = 0.3


def bake(kind, name, cs, **kw):
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=False)
    img.colorspace_settings.name = cs
    node = nt.nodes.new("ShaderNodeTexImage")
    node.image = img
    for n in nt.nodes:
        n.select = False
    node.select = True
    nt.nodes.active = node
    select_only([stone])
    bpy.ops.object.bake(type=kind, margin=12, margin_type="EXTEND", use_clear=True, **kw)
    nt.nodes.remove(node)
    path = f"{WORK}/{name}.png"
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    print("baked", name, path)


bake("DIFFUSE", "stone_bc", "sRGB", pass_filter={"COLOR"})
bake("ROUGHNESS", "stone_rough", "Non-Color")
bake("NORMAL", "stone_normal_gl", "Non-Color", normal_space="TANGENT")
cy.samples = 64
bake("AO", "stone_ao", "Non-Color")

# ---------------------------------------------------------------- clean final materials
def simple_mat(name, color, rough, metal=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = color
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    return m


MATS = {
    "M_Well_Stone": simple_mat("M_Well_Stone", (0.5, 0.5, 0.5, 1), 0.8),
    "M_Well_Wood": simple_mat("M_Well_Wood", (0.4, 0.3, 0.2, 1), 0.85),
    "M_Well_Mortar": simple_mat("M_Well_Mortar", (0.07, 0.065, 0.06, 1), 0.95),
    "M_Well_Iron": simple_mat("M_Well_Iron", (0.16, 0.15, 0.15, 1), 0.4, 0.9),
    "M_Well_Rope": simple_mat("M_Well_Rope", (0.32, 0.22, 0.12, 1), 0.92),
    "M_Well_Water": simple_mat("M_Well_Water", (0.02, 0.06, 0.08, 1), 0.05),
}

# drop the original tiled UV on stone; atlas becomes UV0 named "UVMap"
sme.uv_layers.remove(sme.uv_layers["UVMap"])
sme.uv_layers["Atlas"].name = "UVMap"
if "blk" in sme.color_attributes:
    sme.color_attributes.remove(sme.color_attributes["blk"])

GROUPS = {
    "M_Well_Stone": ["Well_Stone"],
    "M_Well_Wood": ["Post_L", "Post_R", "Crossbeam", "Roof_A", "Roof_B", "Ridge_Cap",
                    "Brace_L", "Brace_R", "Windlass", "Crank_Handle", "Bucket"],
    "M_Well_Mortar": ["Well_Mortar"],
    "M_Well_Iron": ["Crank_Arm", "Bucket_Hoop1", "Bucket_Hoop2"],
    "M_Well_Rope": ["Rope", "Coil"],
    "M_Well_Water": ["Well_Water"],
}
everything = []
for mat_name, names in GROUPS.items():
    for n in names:
        o = O[n]
        select_only([o])
        for md in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=md.name)
        if "blk" in o.data.color_attributes:
            o.data.color_attributes.remove(o.data.color_attributes["blk"])
        o.data.materials.clear()
        o.data.materials.append(MATS[mat_name])
        if not o.data.uv_layers:  # water disc: needs a UV set to export cleanly
            o.data.uv_layers.new(name="UVMap")
        if o.data.uv_layers[0].name != "UVMap":
            o.data.uv_layers[0].name = "UVMap"
        everything.append(o)

main = O["Well_Stone"]
select_only(everything, main)
bpy.ops.object.join()
main.name = "SM_Well_01"
main.data.name = "SM_Well_01"
print("final slots", [s.material.name for s in main.material_slots], "polys", len(main.data.polygons))
bpy.ops.object.mode_set(mode="OBJECT")
bpy.ops.object.origin_set(type="ORIGIN_CURSOR") if False else None

# ---------------------------------------------------------------- collision (UCX_ convex hull)
bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.86, depth=0.9, location=(0, 0, 0.45))
col = bpy.context.active_object
col.name = "UCX_SM_Well_01_01"
col.data.name = col.name

# ---------------------------------------------------------------- export
select_only([main, col], main)
bpy.ops.export_scene.fbx(
    filepath=f"{OUT}/SM_Well_01.fbx",
    use_selection=True,
    object_types={"MESH"},
    axis_forward="-Y",
    axis_up="Z",
    apply_scale_options="FBX_SCALE_ALL",
    global_scale=1.0,
    mesh_smooth_type="FACE",
    use_tspace=True,
    add_leaf_bones=False,
    path_mode="STRIP",
)
dims = main.dimensions
print("EXPORT DONE dims(m)", tuple(round(v, 3) for v in dims))
