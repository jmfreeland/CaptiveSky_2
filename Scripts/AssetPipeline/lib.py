"""Asset toolkit (runs INSIDE Blender, headless). See ASSET_CREATION.md.

An asset script defines NAME (folder, e.g. "BirdBath"), optional BAKE_SIZE, and build(A) where A is an
`Asset`. Parts are authored in metres, Z up, origin at ground centre. Materials are chosen by key:

  library (tiled textures)  wood clay straw strawcoil parchment
  stone                     baked per asset: stone-face + moss shader -> atlas (T_<Name>_Stone_*)
  plain                     iron rope char water brass ember rune lanternglow ...
"""
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

from config import ROOT, SRC_TEX, WORK  # noqa: E402  (paths: see config.py)

TILED = {  # key: (texture base, tile metres)
    "wood": ("T_Lib_Wood", 1.0),
    "clay": ("T_Lib_Clay", 0.6),
    "straw": ("T_Lib_Straw", 0.5),
    "strawcoil": ("T_Lib_Strawcoil", 0.4),
    "parchment": ("T_Lib_Parchment", 0.5),
    "canvas": ("T_Lib_Canvas", 0.35),
    "moss": ("T_Lib_Moss", 0.4),
}
PLAIN = {
    "iron": dict(BaseColor=[0.16, 0.15, 0.15], Roughness=0.4, Metallic=0.9),
    "rope": dict(BaseColor=[0.32, 0.22, 0.12], Roughness=0.92),
    "char": dict(BaseColor=[0.03, 0.028, 0.026], Roughness=0.95),
    "ash": dict(BaseColor=[0.16, 0.15, 0.14], Roughness=1.0),
    "water": dict(BaseColor=[0.02, 0.06, 0.08], Roughness=0.05),
    "brass": dict(BaseColor=[0.62, 0.45, 0.18], Roughness=0.35, Metallic=1.0),
    "leather": dict(BaseColor=[0.14, 0.08, 0.05], Roughness=0.7),
    "dirt": dict(BaseColor=[0.12, 0.085, 0.055], Roughness=1.0),
    "ember": dict(BaseColor=[0.05, 0.02, 0.01], Roughness=0.8, Emissive=[1.0, 0.35, 0.08], EmissiveStrength=6.0),
    "rune": dict(BaseColor=[0.02, 0.03, 0.04], Roughness=0.6, Emissive=[0.35, 0.85, 1.0], EmissiveStrength=8.0),
    "lanternglow": dict(BaseColor=[0.1, 0.07, 0.03], Roughness=0.5, Emissive=[1.0, 0.62, 0.22], EmissiveStrength=5.0),
    "mortar": dict(BaseColor=[0.07, 0.065, 0.06], Roughness=0.95),
    "coal": dict(BaseColor=[0.04, 0.02, 0.015], Roughness=0.9, Emissive=[1.0, 0.28, 0.05], EmissiveStrength=7.0),
    "awning": dict(BaseColor=[0.52, 0.11, 0.09], Roughness=0.9),
    "cloth_blue": dict(BaseColor=[0.12, 0.2, 0.34], Roughness=0.9),
    "felt": dict(BaseColor=[0.18, 0.13, 0.09], Roughness=0.95),
    "mushcap": dict(BaseColor=[0.62, 0.1, 0.07], Roughness=0.55),
    "mushstem": dict(BaseColor=[0.82, 0.77, 0.66], Roughness=0.7),
    "pumpkin": dict(BaseColor=[0.75, 0.32, 0.05], Roughness=0.6),
}


# ------------------------------------------------------------------ UV helpers (manual, no ops)
def _uvlayer(bm):
    for n in list(bm.loops.layers.uv.keys()):
        bm.loops.layers.uv.remove(bm.loops.layers.uv[n])
    return bm.loops.layers.uv.new("UVMap")


def uv_box(bm, tile, grain="Z", off=(0.0, 0.0)):
    uvl = _uvlayer(bm)
    g = {"X": 0, "Y": 1, "Z": 2}[grain]
    for f in bm.faces:
        n = f.normal
        d = max(range(3), key=lambda i: abs(n[i]))
        a, b = [i for i in range(3) if i != d]
        if g in (a, b):
            a, b = [i for i in (a, b) if i != g][0], g
        for l in f.loops:
            p = l.vert.co
            l[uvl].uv = (p[a] / tile + off[0], p[b] / tile + off[1])


def uv_cyl(bm, tile, off=(0.0, 0.0)):
    """Cylindrical about Z with an integer number of tiles round, so the wrap is seamless."""
    uvl = _uvlayer(bm)
    rmax = max((math.hypot(v.co.x, v.co.y) for v in bm.verts), default=0.1)
    nt = max(1, round(2 * math.pi * rmax / tile))
    for f in bm.faces:
        if abs(f.normal.z) > 0.85:
            for l in f.loops:
                l[uvl].uv = (l.vert.co.x / tile + off[0], l.vert.co.y / tile + off[1])
            continue
        us = [(math.atan2(l.vert.co.y, l.vert.co.x) / (2 * math.pi)) % 1.0 for l in f.loops]
        if max(us) - min(us) > 0.5:
            us = [u + 1.0 if u < 0.5 else u for u in us]
        for l, u in zip(f.loops, us):
            l[uvl].uv = (u * nt + off[0], l.vert.co.z / tile + off[1])


def auto_smooth(bm, angle=40.0, smooth=True):
    for f in bm.faces:
        f.smooth = smooth
    if not smooth:
        return
    lim = math.radians(angle)
    for e in bm.edges:
        if len(e.link_faces) == 2 and e.link_faces[0].normal.angle(e.link_faces[1].normal, 0.0) > lim:
            e.smooth = False


# ------------------------------------------------------------------ geometry builders (return bmesh)
def bm_box(sx, sy, sz, bevel=0.0):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co.x *= sx
        v.co.y *= sy
        v.co.z *= sz
    if bevel > 0:
        bmesh.ops.bevel(bm, geom=list(bm.verts) + list(bm.edges) + list(bm.faces), offset=bevel,
                        segments=2, affect="EDGES")
    return bm


def bm_lathe(profile, segs=32):
    """Revolve [(r, z), ...] about Z. r==0 points collapse to an apex. Make the profile closed."""
    bm = bmesh.new()
    rings = []
    for r, z in profile:
        if r < 1e-6:
            v = bm.verts.new((0, 0, z))
            rings.append([v] * segs)
        else:
            rings.append([bm.verts.new((r * math.cos(2 * math.pi * j / segs),
                                        r * math.sin(2 * math.pi * j / segs), z)) for j in range(segs)])
    for i in range(len(profile) - 1):
        for j in range(segs):
            k = (j + 1) % segs
            vs = [rings[i][j], rings[i][k], rings[i + 1][k], rings[i + 1][j]]
            uniq = []
            for v in vs:
                if v not in uniq:
                    uniq.append(v)
            if len(uniq) >= 3:
                try:
                    bm.faces.new(uniq)
                except ValueError:
                    pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    return bm


def bm_cyl(r, h, segs=24, r2=None, bevel=0.0):
    r2 = r if r2 is None else r2
    bm = bm_lathe([(0, -h / 2), (r, -h / 2), (r2, h / 2), (0, h / 2)], segs)
    return bm


def bm_tube(path, r, segs=8, taper=None):
    """Tube along a polyline of Vectors with parallel-transport frames; end caps."""
    bm = bmesh.new()
    pts = [Vector(p) for p in path]
    rings = []
    up = Vector((0, 0, 1))
    prev_t = None
    nrm = None
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        if nrm is None:
            nrm = t.cross(up)
            if nrm.length < 1e-4:
                nrm = t.cross(Vector((1, 0, 0)))
            nrm.normalize()
        else:
            nrm = (nrm - t * nrm.dot(t)).normalized()
        bn = t.cross(nrm).normalized()
        rr = r * (taper(i / max(1, len(pts) - 1)) if taper else 1.0)
        rings.append([bm.verts.new(p + (nrm * math.cos(2 * math.pi * j / segs) + bn * math.sin(2 * math.pi * j / segs)) * rr)
                      for j in range(segs)])
    for i in range(len(rings) - 1):
        for j in range(segs):
            k = (j + 1) % segs
            bm.faces.new([rings[i][j], rings[i][k], rings[i + 1][k], rings[i + 1][j]])
    bm.faces.new(rings[0][::-1])
    bm.faces.new(rings[-1])
    return bm


def bm_rock(rx, ry, rz, seed=0, sub=2, rough=0.14, flat_bottom=0.6):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=1.0)
    off = Vector((seed * 7.13, seed * 3.71, seed * 1.37))
    for v in bm.verts:
        n = noise.noise(v.co * 1.4 + off)
        n2 = noise.noise(v.co * 3.2 + off)
        v.co *= 1.0 + rough * n + rough * 0.4 * n2
        v.co.x *= rx
        v.co.y *= ry
        v.co.z *= rz
        if v.co.z < -flat_bottom * rz:
            v.co.z = -flat_bottom * rz
    return bm


def bm_torus(R, r, segs=32, rsegs=10):
    """Ring of tube radius r around a circle of radius R in the XY plane."""
    bm = bmesh.new()
    verts = []
    for i in range(segs):
        a = 2 * math.pi * i / segs
        c = Vector((math.cos(a), math.sin(a), 0))
        row = []
        for j in range(rsegs):
            b = 2 * math.pi * j / rsegs
            row.append(bm.verts.new(c * (R + r * math.cos(b)) + Vector((0, 0, r * math.sin(b)))))
        verts.append(row)
    for i in range(segs):
        i2 = (i + 1) % segs
        for j in range(rsegs):
            j2 = (j + 1) % rsegs
            bm.faces.new([verts[i][j], verts[i2][j], verts[i2][j2], verts[i][j2]])
    return bm


def bm_grid(points, thick=0.006):
    """Quad surface through rows x cols of Vectors, solidified to a thin closed sheet (cloth, awning)."""
    bm = bmesh.new()
    rows = [[bm.verts.new(p) for p in row] for row in points]
    for i in range(len(rows) - 1):
        for j in range(len(rows[i]) - 1):
            bm.faces.new([rows[i][j], rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bmesh.ops.solidify(bm, geom=bm.faces[:], thickness=thick)
    return bm


def bm_block(a0, a1, z0, z1, rin, rout, rng, over=0.0, jit=0.012):
    """One jittered wedge block of a round wall (angles a0..a1 about Z), 2 segments across for curvature."""
    bm = bmesh.new()
    angs = (a0, (a0 + a1) / 2, a1)
    V = {}
    for ai, a in enumerate(angs):
        for ri, r in enumerate((rin - over, rout + over)):
            for zi, z in enumerate((z0, z1)):
                rj = r + rng.uniform(-jit, jit) * (1 if ri else 0.6)
                zj = z + (rng.uniform(-jit / 2, jit / 2) if zi else 0)
                V[ai, ri, zi] = bm.verts.new((rj * math.cos(a), rj * math.sin(a), zj))
    for k in (0, 1):
        bm.faces.new((V[k, 1, 0], V[k + 1, 1, 0], V[k + 1, 1, 1], V[k, 1, 1]))
        bm.faces.new((V[k, 0, 0], V[k, 0, 1], V[k + 1, 0, 1], V[k + 1, 0, 0]))
        bm.faces.new((V[k, 0, 1], V[k, 1, 1], V[k + 1, 1, 1], V[k + 1, 0, 1]))
        bm.faces.new((V[k, 0, 0], V[k + 1, 0, 0], V[k + 1, 1, 0], V[k, 1, 0]))
    bm.faces.new((V[0, 0, 0], V[0, 1, 0], V[0, 1, 1], V[0, 0, 1]))
    bm.faces.new((V[2, 0, 0], V[2, 0, 1], V[2, 1, 1], V[2, 1, 0]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bmesh.ops.bevel(bm, geom=list(bm.verts) + list(bm.edges) + list(bm.faces), offset=0.011, segments=2, affect="EDGES")
    return bm


def align(direction, up="Y"):
    """Euler degrees (XYZ) that point a part's local +Z along `direction` (legs, braces, tilted tubes)."""
    return tuple(math.degrees(a) for a in Vector(direction).to_track_quat("Z", up).to_euler())


# ------------------------------------------------------------------ materials (preview / manifest)
def _principled(mat):
    mat.use_nodes = True
    return next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")


def _sock(n, name):
    return next(s for s in n.inputs if s.name == name and s.enabled)


def _osock(n, name):
    return next(s for s in n.outputs if s.name == name and s.enabled)


def build_stone_moss_material(name="Mat_StoneMoss", damp_h=0.7, thresh=0.42):
    """Stone face + moss layer shader (used only as the BAKE SOURCE)."""
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    N, L = nt.nodes, nt.links
    N.clear()

    def node(t, loc, **kw):
        n = N.new(t)
        n.location = loc
        for k, v in kw.items():
            setattr(n, k, v)
        return n

    def img(path, cs, loc, vec):
        n = node("ShaderNodeTexImage", loc)
        n.image = bpy.data.images.load(f"{SRC_TEX}/{path}", check_existing=True)
        n.image.colorspace_settings.name = cs
        L.new(vec, n.inputs["Vector"])
        return n

    out = node("ShaderNodeOutputMaterial", (2200, 0))
    bsdf = node("ShaderNodeBsdfPrincipled", (1900, 0))
    L.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    uvn = node("ShaderNodeUVMap", (-1600, 0))
    uvn.uv_map = "UVMap"
    mp = node("ShaderNodeMapping", (-1400, -500))
    mp.inputs["Scale"].default_value = (3, 3, 3)
    L.new(uvn.outputs["UV"], mp.inputs["Vector"])
    s_alb = img("stone_face/albedo.png", "sRGB", (-1000, 700), uvn.outputs["UV"])
    s_rg = img("stone_face/roughness.png", "Non-Color", (-1000, 450), uvn.outputs["UV"])
    s_nm = img("stone_face/normal_gl.png", "Non-Color", (-1000, 200), uvn.outputs["UV"])
    m_alb = img("moss/albedo.png", "sRGB", (-1000, -400), mp.outputs["Vector"])
    m_rg = img("moss/roughness.png", "Non-Color", (-1000, -650), mp.outputs["Vector"])
    m_nm = img("moss/normal_gl.png", "Non-Color", (-1000, -900), mp.outputs["Vector"])
    blk = node("ShaderNodeAttribute", (-1600, 900))
    blk.attribute_name = "blk"
    blk.attribute_type = "GEOMETRY"
    hs = node("ShaderNodeHueSaturation", (-700, 700))
    hs.inputs["Value"].default_value = 0.62
    hs.inputs["Saturation"].default_value = 0.9
    L.new(s_alb.outputs["Color"], hs.inputs["Color"])
    ramp = node("ShaderNodeValToRGB", (-1300, 900))
    ramp.color_ramp.elements[0].color = (0.72, 0.70, 0.66, 1)
    ramp.color_ramp.elements[1].color = (1.0, 0.98, 0.92, 1)
    L.new(blk.outputs["Fac"], ramp.inputs["Fac"])
    tint = node("ShaderNodeMix", (-450, 700), data_type="RGBA", blend_type="MULTIPLY")
    _sock(tint, "Factor").default_value = 1.0
    L.new(hs.outputs["Color"], _sock(tint, "A"))
    L.new(ramp.outputs["Color"], _sock(tint, "B"))
    geo = node("ShaderNodeNewGeometry", (-1400, 1300))
    dot = node("ShaderNodeVectorMath", (-1150, 1300), operation="DOT_PRODUCT")
    dot.inputs[1].default_value = (0, 0, 1)
    L.new(geo.outputs["Normal"], dot.inputs[0])
    up = node("ShaderNodeMapRange", (-900, 1300))
    up.inputs["From Min"].default_value = 0.25
    up.inputs["From Max"].default_value = 0.95
    L.new(dot.outputs["Value"], up.inputs["Value"])
    ao = node("ShaderNodeAmbientOcclusion", (-1150, 1050))
    ao.inputs["Distance"].default_value = 0.2
    cav = node("ShaderNodeMapRange", (-900, 1050))
    cav.inputs["From Min"].default_value = 1.0
    cav.inputs["From Max"].default_value = 0.35
    L.new(ao.outputs["AO"], cav.inputs["Value"])
    sepz = node("ShaderNodeSeparateXYZ", (-1150, 800))
    L.new(geo.outputs["Position"], sepz.inputs["Vector"])
    damp = node("ShaderNodeMapRange", (-900, 800))
    damp.inputs["From Min"].default_value = damp_h
    damp.inputs["From Max"].default_value = 0.0
    L.new(sepz.outputs["Z"], damp.inputs["Value"])
    tcn = node("ShaderNodeTexCoord", (-1400, 1550))
    noi = node("ShaderNodeTexNoise", (-1150, 1550))
    noi.inputs["Scale"].default_value = 9.0
    noi.inputs["Detail"].default_value = 7.0
    L.new(tcn.outputs["Object"], noi.inputs["Vector"])

    def mth(op, a, b, loc):
        n = node("ShaderNodeMath", loc, operation=op)
        for i, v in enumerate((a, b)):
            if isinstance(v, (int, float)):
                n.inputs[i].default_value = v
            else:
                L.new(v, n.inputs[i])
        return n.outputs[0]

    t_up = mth("MULTIPLY", _osock(up, "Result"), 0.5, (-650, 1300))
    t_cv = mth("MULTIPLY", _osock(cav, "Result"), 0.7, (-650, 1100))
    t_dp = mth("MULTIPLY", _osock(damp, "Result"), 0.9, (-650, 900))
    t_nz = mth("MULTIPLY", mth("SUBTRACT", noi.outputs["Fac"], 0.5, (-850, 1550)), 0.8, (-650, 1550))
    t_bk = mth("MULTIPLY", mth("SUBTRACT", blk.outputs["Fac"], 0.5, (-850, 1700)), 0.35, (-650, 1700))
    s = mth("ADD", mth("ADD", t_up, t_cv, (-400, 1200)), mth("ADD", t_dp, t_nz, (-400, 1000)), (-200, 1100))
    s = mth("ADD", s, t_bk, (0, 1100))
    s = mth("SUBTRACT", s, thresh, (200, 1100))
    mr = node("ShaderNodeValToRGB", (450, 1100))
    mr.color_ramp.elements[0].position = 0.0
    mr.color_ramp.elements[1].position = 0.22
    L.new(s, mr.inputs["Fac"])
    mask = mr.outputs["Color"]

    def mixer(dt, a, b, loc):
        n = node("ShaderNodeMix", loc, data_type=dt)
        L.new(mask, _sock(n, "Factor"))
        L.new(a, _sock(n, "A"))
        L.new(b, _sock(n, "B"))
        return _osock(n, "Result")

    mhs = node("ShaderNodeHueSaturation", (-700, -400))
    mhs.inputs["Saturation"].default_value = 0.68
    mhs.inputs["Value"].default_value = 0.72
    L.new(m_alb.outputs["Color"], mhs.inputs["Color"])
    L.new(mixer("RGBA", _osock(tint, "Result"), mhs.outputs["Color"], (900, 500)), bsdf.inputs["Base Color"])
    L.new(mixer("FLOAT", s_rg.outputs["Color"], m_rg.outputs["Color"], (900, 250)), bsdf.inputs["Roughness"])
    snm = node("ShaderNodeNormalMap", (-600, 200))
    snm.inputs["Strength"].default_value = 0.55
    L.new(s_nm.outputs["Color"], snm.inputs["Color"])
    mnm = node("ShaderNodeNormalMap", (-600, -900))
    mnm.inputs["Strength"].default_value = 0.8
    L.new(m_nm.outputs["Color"], mnm.inputs["Color"])
    L.new(mixer("VECTOR", snm.outputs["Normal"], mnm.outputs["Normal"], (900, -300)), bsdf.inputs["Normal"])
    return m


def plain_material(name, spec):
    m = bpy.data.materials.new(name)
    b = _principled(m)
    b.inputs["Base Color"].default_value = (*spec["BaseColor"], 1)
    b.inputs["Roughness"].default_value = spec.get("Roughness", 0.5)
    b.inputs["Metallic"].default_value = spec.get("Metallic", 0.0)
    if "Emissive" in spec:
        b.inputs["Emission Color"].default_value = (*spec["Emissive"], 1)
        b.inputs["Emission Strength"].default_value = spec.get("EmissiveStrength", 1.0)
    return m


def textured_material(mat, asset_dir, spec):
    """Rebuild a textured material from exported PNGs (DirectX normal -> flipped for Blender)."""
    nt = mat.node_tree
    b = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")

    def tex(path, cs, loc):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(os.path.normpath(os.path.join(asset_dir, path.split(" ")[0])), check_existing=True)
        n.image.colorspace_settings.name = cs
        n.location = loc
        return n

    bc = tex(spec["BaseColor"], "sRGB", (-900, 300))
    nrm = tex(spec["Normal"], "Non-Color", (-900, -300))
    orm = tex(spec["ORM"], "Non-Color", (-900, 0))
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    sep.location = (-600, 0)
    nt.links.new(orm.outputs["Color"], sep.inputs["Color"])
    comb = nt.nodes.new("ShaderNodeCombineColor")
    comb.location = (-450, 150)
    for i in ("Red", "Green", "Blue"):
        nt.links.new(sep.outputs["Red"], comb.inputs[i])
    mul = nt.nodes.new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.location = (-300, 300)
    _sock(mul, "Factor").default_value = 1.0
    nt.links.new(bc.outputs["Color"], _sock(mul, "A"))
    nt.links.new(comb.outputs["Color"], _sock(mul, "B"))
    nt.links.new(_osock(mul, "Result"), b.inputs["Base Color"])
    nt.links.new(sep.outputs["Green"], b.inputs["Roughness"])
    sn = nt.nodes.new("ShaderNodeSeparateColor")
    sn.location = (-650, -300)
    nt.links.new(nrm.outputs["Color"], sn.inputs["Color"])
    inv = nt.nodes.new("ShaderNodeMath")
    inv.operation = "SUBTRACT"
    inv.inputs[0].default_value = 1.0
    inv.location = (-500, -380)
    nt.links.new(sn.outputs["Green"], inv.inputs[1])
    cn = nt.nodes.new("ShaderNodeCombineColor")
    cn.location = (-350, -300)
    nt.links.new(sn.outputs["Red"], cn.inputs["Red"])
    nt.links.new(inv.outputs[0], cn.inputs["Green"])
    nt.links.new(sn.outputs["Blue"], cn.inputs["Blue"])
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nm.location = (-150, -300)
    nt.links.new(cn.outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], b.inputs["Normal"])


# ------------------------------------------------------------------ the Asset builder
class Asset:
    def __init__(self, folder, bake_size=2048):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        self.folder = folder
        self.name = f"SM_{folder}_01"
        self.dir = f"{ROOT}/{folder}"
        self.bake_size = bake_size
        self.description = ""
        self.moss = (0.7, 0.42)  # (dampness height in m, moss threshold: higher = less moss)
        self.parts = []  # (object, matkey)
        self.coll = []
        self.rng = random.Random(sum(ord(c) for c in folder))
        os.makedirs(f"{self.dir}/Textures", exist_ok=True)
        os.makedirs(WORK, exist_ok=True)
        self._uid = 0

    # -- helpers
    def tile(self, mat):
        return TILED[mat][1] if mat in TILED else 1.0

    def _finish(self, bm, name, mat, loc, rot, uv, grain, tile, smooth, sharp):
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        tile = tile or self.tile(mat)
        off = (self.rng.random(), self.rng.random())
        if uv == "box":
            uv_box(bm, tile, grain, off)
        elif uv == "cyl":
            uv_cyl(bm, tile, off)
        auto_smooth(bm, sharp, smooth)
        if mat == "stone":
            layer = bm.loops.layers.float_color.new("blk")
            rv = self.rng.random()
            for f in bm.faces:
                for l in f.loops:
                    l[layer] = (rv, rv, rv, 1.0)
        M = Matrix.Translation(loc) @ Euler([math.radians(a) for a in rot], "XYZ").to_matrix().to_4x4()
        bmesh.ops.transform(bm, matrix=M, verts=bm.verts)
        self._uid += 1
        me = bpy.data.meshes.new(f"{name}_{self._uid}")
        bm.to_mesh(me)
        bm.free()
        obj = bpy.data.objects.new(f"{name}_{self._uid}", me)
        bpy.context.scene.collection.objects.link(obj)
        self.parts.append((obj, mat))
        return obj

    # -- public primitives
    def box(self, mat, size, loc=(0, 0, 0), rot=(0, 0, 0), bevel=0.0, grain="Z", tile=None, name="box", smooth=False):
        return self._finish(bm_box(*size, bevel=bevel), name, mat, loc, rot, "box", grain, tile, smooth, 40)

    def cyl(self, mat, r, h, loc=(0, 0, 0), rot=(0, 0, 0), r2=None, segs=24, uv="cyl", grain="Z", tile=None, name="cyl", smooth=True):
        return self._finish(bm_cyl(r, h, segs, r2), name, mat, loc, rot, uv, grain, tile, smooth, 40)

    def lathe(self, mat, profile, loc=(0, 0, 0), rot=(0, 0, 0), segs=32, tile=None, name="lathe", smooth=True, sharp=35):
        return self._finish(bm_lathe(profile, segs), name, mat, loc, rot, "cyl", "Z", tile, smooth, sharp)

    def tube(self, mat, path, r, segs=8, loc=(0, 0, 0), rot=(0, 0, 0), taper=None, tile=None, grain="Z", name="tube"):
        return self._finish(bm_tube(path, r, segs, taper), name, mat, loc, rot, "box", grain, tile, True, 45)

    def rock(self, mat, radii, loc=(0, 0, 0), rot=(0, 0, 0), seed=1, sub=2, rough=0.14, flat_bottom=0.6, tile=None, name="rock"):
        return self._finish(bm_rock(*radii, seed=seed, sub=sub, rough=rough, flat_bottom=flat_bottom), name, mat, loc, rot, "box", "Z", tile, True, 70)

    def torus(self, mat, R, r, loc=(0, 0, 0), rot=(0, 0, 0), segs=32, rsegs=10, tile=None, name="torus"):
        return self._finish(bm_torus(R, r, segs, rsegs), name, mat, loc, rot, "box", "Z", tile, True, 50)

    def cloth(self, mat, points, thick=0.006, loc=(0, 0, 0), rot=(0, 0, 0), tile=None, name="cloth"):
        """points: rows x cols of Vectors (a draped / curved surface); becomes a thin closed sheet."""
        return self._finish(bm_grid(points, thick), name, mat, loc, rot, "box", "Z", tile, True, 60)

    def block_ring(self, mat, r_in, r_out, courses, course_h, per_course=13, cap_over=0.02, tile=1.2, name="block"):
        """A round dry-stone wall of individually jittered blocks, staggered per course; the top course is a slightly proud capstone."""
        made = []
        for c in range(courses):
            n = per_course + (c % 2)
            ws = [self.rng.uniform(0.75, 1.3) for _ in range(n)]
            tot = sum(ws)
            ws = [w / tot * 2 * math.pi for w in ws]
            a = self.rng.uniform(0, 2 * math.pi)
            z0, z1 = c * course_h + 0.008, (c + 1) * course_h - 0.004
            cap = c == courses - 1
            for w in ws:
                bm = bm_block(a + 0.007, a + w - 0.007, z0, z1 + (cap_over if cap else 0), r_in - (cap_over if cap else 0), r_out, self.rng, over=(0.015 if cap else 0))
                made.append(self._finish(bm, name, mat, (0, 0, 0), (0, 0, 0), "cyl", "Z", tile, True, 40))
                a += w
        return made

    # -- collision
    def collide_box(self, size, loc=(0, 0, 0), rot=(0, 0, 0)):
        self.coll.append(("UBX", bm_box(*size), loc, rot))

    def collide_cyl(self, r, h, loc=(0, 0, 0), rot=(0, 0, 0), segs=14):
        self.coll.append(("UCX", bm_cyl(r, h, segs), loc, rot))

    # -- stone bake -------------------------------------------------------------------------------
    def _bake_stone(self, stone_objs):
        sc = bpy.context.scene
        for ob in stone_objs:
            assert [u.name for u in ob.data.uv_layers] == ["UVMap"], (ob.name, [u.name for u in ob.data.uv_layers])
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        for o in stone_objs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = stone_objs[0]
        if len(stone_objs) > 1:
            bpy.ops.object.join()
        stone = bpy.context.view_layer.objects.active
        me = stone.data
        mat = build_stone_moss_material(damp_h=self.moss[0], thresh=self.moss[1])
        me.materials.clear()
        me.materials.append(mat)
        for p in me.polygons:
            p.material_index = 0
        atlas = me.uv_layers.new(name="Atlas")
        me.uv_layers.active = atlas
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.003, scale_to_bounds=False)
        bpy.ops.uv.pack_islands(margin=0.003)
        bpy.ops.object.mode_set(mode="OBJECT")
        me.uv_layers.active = me.uv_layers["Atlas"]
        me.uv_layers["Atlas"].active_render = True

        sc.render.engine = "CYCLES"
        sc.cycles.samples = 16
        try:
            prefs = bpy.context.preferences.addons["cycles"].preferences
            for dev in ("OPTIX", "CUDA"):
                try:
                    prefs.compute_device_type = dev
                    prefs.get_devices()
                    gpus = [d for d in prefs.devices if d.type == dev]
                    if gpus:
                        for d in prefs.devices:
                            d.use = d.type == dev
                        sc.cycles.device = "GPU"
                        break
                except Exception:
                    continue
        except Exception as e:
            print("gpu setup failed", e)
        if sc.world is None:
            sc.world = bpy.data.worlds.new("W")
        sc.world.light_settings.distance = 0.25
        nt = mat.node_tree
        size = self.bake_size

        def bake(kind, key, cs, **kw):
            img = bpy.data.images.new(f"{self.folder}_{key}", size, size, alpha=False)
            img.colorspace_settings.name = cs
            node = nt.nodes.new("ShaderNodeTexImage")
            node.image = img
            for n in nt.nodes:
                n.select = False
            node.select = True
            nt.nodes.active = node
            for o in bpy.context.view_layer.objects:
                o.select_set(False)
            stone.select_set(True)
            bpy.context.view_layer.objects.active = stone
            bpy.ops.object.bake(type=kind, margin=10, margin_type="EXTEND", use_clear=True, **kw)
            nt.nodes.remove(node)
            img.filepath_raw = f"{WORK}/{self.folder}_stone_{key}.png"
            img.file_format = "PNG"
            img.save()
            print("baked", key)

        bake("DIFFUSE", "bc", "sRGB", pass_filter={"COLOR"})
        bake("ROUGHNESS", "rough", "Non-Color")
        bake("NORMAL", "normal_gl", "Non-Color", normal_space="TANGENT")
        sc.cycles.samples = 48
        bake("AO", "ao", "Non-Color")
        me.uv_layers.remove(me.uv_layers["UVMap"])
        me.uv_layers["Atlas"].name = "UVMap"
        if "blk" in me.color_attributes:
            me.color_attributes.remove(me.color_attributes["blk"])
        return stone

    # -- finalise ---------------------------------------------------------------------------------
    def finalize(self):
        sc = bpy.context.scene
        stone_objs = [o for o, m in self.parts if m == "stone"]
        others = [(o, m) for o, m in self.parts if m != "stone"]
        parts = []
        if stone_objs:
            parts.append((self._bake_stone(stone_objs), "stone"))
        parts += others

        mats = {}
        manifest_mats = {}

        def matfor(key):
            if key in mats:
                return mats[key]
            if key == "stone":
                nm = f"M_{self.folder}_Stone"
                spec = {"BaseColor": f"Textures/T_{self.folder}_Stone_BC.png (sRGB)",
                        "Normal": f"Textures/T_{self.folder}_Stone_N.png (DirectX, compression: Normalmap)",
                        "ORM": f"Textures/T_{self.folder}_Stone_ORM.png (R=AO, G=Roughness, B=Metallic; sRGB off, Masks)",
                        "notes": "moss, stone variation and crevice AO baked into a unique 0-1 atlas; no tiling."}
                m = bpy.data.materials.new(nm)
                _principled(m)
            elif key in TILED:
                base, tile = TILED[key]
                nm = f"M_Lib_{key.capitalize()}"
                spec = {"BaseColor": f"../Library/Textures/{base}_BC.png (sRGB, tiles)",
                        "Normal": f"../Library/Textures/{base}_N.png (DirectX, tiles)",
                        "ORM": f"../Library/Textures/{base}_ORM.png (R=AO, G=Roughness, B=Metallic; sRGB off)",
                        "uv_tile_metres": tile}
                m = bpy.data.materials.new(nm)
                _principled(m)
            else:
                nm = f"M_Lib_{key.capitalize()}"
                spec = dict(PLAIN[key])
                m = plain_material(nm, spec)
            mats[key] = m
            manifest_mats[nm] = spec
            return m

        for o, key in parts:
            for md in list(o.modifiers):
                bpy.context.view_layer.objects.active = o
                bpy.ops.object.modifier_apply(modifier=md.name)
            o.data.materials.clear()
            o.data.materials.append(matfor(key))
            if not o.data.uv_layers:
                o.data.uv_layers.new(name="UVMap")
            o.data.uv_layers[0].name = "UVMap"
            for ca in list(o.data.color_attributes):
                o.data.color_attributes.remove(ca)

        objs = [o for o, _ in parts]
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        for o in objs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = objs[0]
        if len(objs) > 1:
            bpy.ops.object.join()
        main = bpy.context.view_layer.objects.active
        main.name = self.name
        main.data.name = self.name
        zmin = min(v.co.z for v in main.data.vertices)
        print("zmin", round(zmin, 4), "dims", tuple(round(d, 3) for d in main.dimensions))

        coll_names = []
        for i, (kind, bm, loc, rot) in enumerate(self.coll, 1):
            M = Matrix.Translation(loc) @ Euler([math.radians(a) for a in rot], "XYZ").to_matrix().to_4x4()
            bmesh.ops.transform(bm, matrix=M, verts=bm.verts)
            nm = f"{kind}_{self.name}_{i:02d}"
            me = bpy.data.meshes.new(nm)
            bm.to_mesh(me)
            bm.free()
            co = bpy.data.objects.new(nm, me)
            sc.collection.objects.link(co)
            coll_names.append(nm)

        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        main.select_set(True)
        for n in coll_names:
            bpy.data.objects[n].select_set(True)
        bpy.context.view_layer.objects.active = main
        bpy.ops.export_scene.fbx(
            filepath=f"{self.dir}/{self.name}.fbx", use_selection=True, object_types={"MESH"},
            axis_forward="-Y", axis_up="Z", apply_scale_options="FBX_SCALE_ALL", global_scale=1.0,
            mesh_smooth_type="OFF", use_tspace=True, add_leaf_bones=False, path_mode="STRIP")
        d = main.dimensions
        manifest = {
            "asset": self.name,
            "description": self.description,
            "fbx": f"{self.name}.fbx",
            "units": "centimetres (FBX_SCALE_ALL); origin = ground-centre; Z up; normals exported (Normals Only)",
            "size_cm_xyz": [round(d.x * 100), round(d.y * 100), round(d.z * 100)],
            "triangles_approx": sum(max(1, len(p.vertices) - 2) for p in main.data.polygons),
            "collision": coll_names,
            "uv": "single UV channel 'UVMap'. Stone slot: unique 0-1 atlas. Library slots: tiled (>1) UVs, textures must wrap.",
            "material_slots": [s.material.name for s in main.material_slots],
            "materials": manifest_mats,
            "provenance": "Textures: local ComfyUI (SDXL base 1.0) + derived PBR maps; modelled/baked in Blender 5.2 via Scripts/AssetPipeline in the CaptiveSky_2 repo (see Scripts/AssetPipeline/README.md)",
        }
        json.dump(manifest, open(f"{self.dir}/manifest.json", "w"), indent=2)
        print("EXPORT DONE", self.name, manifest["size_cm_xyz"], "tris", manifest["triangles_approx"], "slots", manifest["material_slots"])
