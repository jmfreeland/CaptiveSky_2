"""Low, sprawling sea-pea foliage for Tideglass's sheltered dune edge."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassSeaPea"
PROVENANCE = "Deterministic collisionless Lathyrus japonicus-inspired procedural geometry authored in Blender 5.2; muted blue-green pinnate foliage and rose-pink pea flowers; no generated textures."
RUNNER_COUNT = 6
LEAF_PAIRS_PER_RUNNER = 3
LEAFLET_PAIRS_PER_LEAF = 3
FLOWERS_PER_RUNNER = 2


def _leaflet(bm, base, tip, width):
    """A softly cupped lance-shaped leaflet with a raised midrib."""
    start = Vector(base)
    end = Vector(tip)
    axis = (end - start).normalized()
    side = axis.cross(Vector((0.0, 0.0, 1.0))).normalized()
    if side.length_squared < 0.5:
        side = Vector((0.0, 1.0, 0.0))
    shoulder = start.lerp(end, 0.48)
    ridge = shoulder + Vector((0.0, 0.0, width * 0.12))
    left = shoulder - side * width * 0.5
    right = shoulder + side * width * 0.5
    verts = [bm.verts.new(point) for point in (start, left, end, right, ridge)]
    bm.faces.new((verts[0], verts[1], verts[4]))
    bm.faces.new((verts[1], verts[2], verts[4]))
    bm.faces.new((verts[2], verts[3], verts[4]))
    bm.faces.new((verts[3], verts[0], verts[4]))


def _pea_bloom(bm, centre, phase):
    """Five rounded petals form a small open, papilionaceous-style bloom."""
    origin = Vector(centre)
    for petal in range(5):
        angle = phase + math.tau * petal / 5.0
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        side = Vector((-radial.y, radial.x, 0.0))
        root = origin + radial * 0.001
        shoulder = origin + radial * 0.005 + Vector((0.0, 0.0, 0.002))
        tip = origin + radial * 0.014 + Vector((0.0, 0.0, 0.006))
        left = shoulder - side * 0.005
        right = shoulder + side * 0.005
        verts = [bm.verts.new(point) for point in (root, left, tip, right)]
        bm.faces.new((verts[0], verts[1], verts[2]))
        bm.faces.new((verts[0], verts[2], verts[3]))


def build(A):
    lib.PLAIN["sea_pea_stem"] = {"BaseColor": [0.15, 0.24, 0.16], "Roughness": 0.91}
    lib.PLAIN["sea_pea_leaf"] = {"BaseColor": [0.24, 0.34, 0.26], "Roughness": 0.9}
    lib.PLAIN["sea_pea_flower"] = {"BaseColor": [0.62, 0.19, 0.28], "Roughness": 0.84}

    leaf_bm = bmesh.new()
    flower_bm = bmesh.new()
    for runner_index in range(RUNNER_COUNT):
        angle = math.tau * runner_index / RUNNER_COUNT + 0.19 * (runner_index % 2)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        lateral = Vector((-radial.y, radial.x, 0.0))
        base = radial * 0.025 + Vector((0.0, 0.0, 0.009))
        end = radial * (0.205 + 0.018 * (runner_index % 3)) + Vector((0.0, 0.0, 0.035 + 0.006 * (runner_index % 2)))
        path = [base, base.lerp(end, 0.34) + Vector((0.0, 0.0, 0.012)),
                base.lerp(end, 0.72) + Vector((0.0, 0.0, 0.008)), end]
        A.tube("sea_pea_stem", path, 0.0032, segs=5,
               taper=lambda t: 1.0 - 0.48 * t,
               name="seaPeaRunner{:02d}".format(runner_index + 1))

        for leaf_index in range(LEAF_PAIRS_PER_RUNNER):
            node_t = 0.19 + 0.22 * leaf_index
            node = base.lerp(end, node_t) + Vector((0.0, 0.0, 0.005))
            for side_sign in (-1, 1):
                leaf_direction = (radial * 0.42 + lateral * side_sign * 0.91
                                  + Vector((0.0, 0.0, 0.22))).normalized()
                leaf_length = 0.067 - 0.005 * leaf_index
                leaf_tip = node + leaf_direction * leaf_length
                A.tube("sea_pea_stem", [node, node.lerp(leaf_tip, 0.55), leaf_tip],
                       0.0018, segs=4, taper=lambda t: 1.0 - 0.55 * t,
                       name="seaPeaLeafRachis{:02d}_{:02d}_{}".format(
                           runner_index + 1, leaf_index + 1, side_sign))
                leaf_axis = (leaf_tip - node).normalized()
                leaf_side = Vector((-leaf_axis.y, leaf_axis.x, 0.0)).normalized()
                for pair_index in range(LEAFLET_PAIRS_PER_LEAF):
                    t = 0.24 + 0.25 * pair_index
                    leaflet_root = node.lerp(leaf_tip, t)
                    length = 0.021 - 0.002 * pair_index
                    for leaflet_side in (-1, 1):
                        leaflet_direction = (leaf_axis * 0.25 + leaf_side * leaflet_side * 0.94
                                             + Vector((0.0, 0.0, 0.08))).normalized()
                        leaflet_tip = leaflet_root + leaflet_direction * length
                        _leaflet(leaf_bm, leaflet_root, leaflet_tip, 0.009)

                # The real plant finishes each leaf in a fine tendril; a short curl keeps the
                # silhouette recognizable without adding an expensive decorative coil mesh.
                curl_centre = leaf_tip + leaf_axis * 0.009
                curl = [curl_centre + Vector((0.004 * math.cos(t), 0.004 * math.sin(t), 0.0015 * t / math.tau))
                        for t in (0.0, math.pi / 2.0, math.pi, 3.0 * math.pi / 2.0, math.tau)]
                A.tube("sea_pea_stem", curl, 0.0008, segs=4,
                       taper=lambda t: 1.0 - 0.7 * t,
                       name="seaPeaTendril{:02d}_{:02d}_{}".format(
                           runner_index + 1, leaf_index + 1, side_sign))

        # Small paired flowers sit just above the low foliage at the runner tip.
        for flower_index in range(FLOWERS_PER_RUNNER):
            flower_centre = end + radial * (0.014 * flower_index) + Vector((0.0, 0.0, 0.014 + flower_index * 0.007))
            _pea_bloom(flower_bm, flower_centre, angle + flower_index * 0.7)

    for bm, name, material in ((leaf_bm, "seaPeaLeaflets", "sea_pea_leaf"),
                               (flower_bm, "seaPeaFlowers", "sea_pea_flower")):
        if bm.faces:
            A._finish(bm, name, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
        else:
            bm.free()

    # Repeated coastal ground cover is static, collisionless, and non-navigable.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
