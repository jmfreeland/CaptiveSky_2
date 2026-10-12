"""Small bracket-fungus colony on a rain-dark fallen branch, with moss and snapped twigs."""
import math

import bmesh


NAME = "BracketFungusLog"


def _shelf(A, width, length, thickness, loc, material, seed):
    """A shallow, domed D-shaped polypore shelf, attached along its straight edge."""
    bm = bmesh.new()
    # A smoother scalloped shelf silhouette survives the close Rat-eye view.
    segments = 24
    top = []
    bottom = []
    for i in range(segments + 1):
        angle = math.pi * i / segments
        wobble = 1.0 + 0.035 * math.sin(3.0 * angle + seed) + 0.018 * math.sin(7.0 * angle - seed)
        x = width * math.cos(angle) * wobble
        y = -length * math.sin(angle) * wobble
        top_z = thickness * 0.5 + 0.012 * math.sin(angle)
        top.append(bm.verts.new((x, y, top_z)))
        bottom.append(bm.verts.new((x, y, -thickness * 0.5)))

    dome = bm.verts.new((0.0, -length * 0.34, thickness * 0.5 + 0.018))
    underside = bm.verts.new((0.0, -length * 0.34, -thickness * 0.5))
    for i in range(segments):
        j = i + 1
        bm.faces.new((dome, top[i], top[j]))
        bm.faces.new((underside, bottom[j], bottom[i]))
    for i in range(segments + 1):
        j = (i + 1) % (segments + 1)
        bm.faces.new((top[i], bottom[i], bottom[j], top[j]))

    A._finish(bm, "polypore", material, loc, (0, 0, 0), "box", "Z", 1.0, True, 55)


def _bracket(A, x, z, scale, seed):
    root_y = -0.155 - 0.018 * (seed % 2)
    _shelf(A, 0.162 * scale, 0.145 * scale, 0.020 * scale,
           (x, root_y, z), "polypore_umber", seed)
    _shelf(A, 0.134 * scale, 0.122 * scale, 0.015 * scale,
           (x, root_y - 0.001, z + 0.011 * scale), "polypore_tan", seed + 0.7)
    cap_width, cap_length, cap_thickness = 0.116 * scale, 0.108 * scale, 0.014 * scale
    cap_z = z + 0.020 * scale
    _shelf(A, cap_width, cap_length, cap_thickness,
           (x, root_y - 0.002, cap_z), "polypore_cream", seed + 1.4)

    # Fine growth lines help the smooth cap read as a living, banded polypore.
    for ring, depth in enumerate((0.30, 0.48, 0.66, 0.82)):
        half_width = cap_width * math.sqrt(1.0 - depth * depth) * 0.88
        if depth <= 0.34:
            dome = depth / 0.34
        else:
            dome = 1.0 - (depth - 0.34) / 0.66
        surface_z = cap_thickness * 0.5 + 0.018 * dome + 0.002
        path = []
        for step in range(9):
            across = -1.0 + 2.0 * step / 8.0
            path.append((across * half_width,
                         -cap_length * depth * math.sqrt(max(0.0, 1.0 - across * across)),
                         surface_z))
        A.tube("polypore_tan", path, 0.002, segs=4,
               loc=(x, root_y - 0.002, cap_z), name="growthRidge_{}".format(ring))


def build(A):
    # One fallen hazel-like limb: the fungus stays a small readable detail, not another stump.
    A.tube("wood", [(-0.98, 0.0, 0.21), (-0.55, 0.015, 0.23), (0.0, 0.0, 0.22),
                     (0.55, -0.015, 0.20), (0.98, 0.0, 0.18)],
           0.19, segs=12, taper=lambda t: 1.08 - 0.34 * t,
           tile=2.0, grain="X", name="fallenBranch")

    # Three overlapping colonies, varied in scale and height along the branch's visible side.
    # Lift the colony toward the branch crown so a low visitor view can read the caps.
    _bracket(A, -0.54, 0.311, 0.98, 2)
    _bracket(A, -0.07, 0.340, 1.14, 3)
    _bracket(A, 0.49, 0.295, 0.86, 4)

    # Broken twig stubs and restrained moss make the host wood feel weathered and inhabited.
    A.tube("wood", [(-0.25, 0.035, 0.31), (-0.34, 0.04, 0.42), (-0.42, 0.04, 0.47)],
           0.043, segs=7, taper=lambda t: 1.0 - 0.82 * t, tile=0.7, name="snappedTwig")
    A.tube("wood", [(0.38, 0.025, 0.29), (0.44, 0.04, 0.37), (0.52, 0.05, 0.40)],
           0.035, segs=7, taper=lambda t: 1.0 - 0.85 * t, tile=0.7, name="snappedTwig")
    for x, y, z, radii, seed in (
            (-0.78, -0.04, 0.33, (0.065, 0.052, 0.018), 7),
            (0.18, 0.045, 0.38, (0.068, 0.054, 0.019), 11),
            (0.76, -0.035, 0.27, (0.050, 0.044, 0.016), 13)):
        A.rock("moss", radii, loc=(x, y, z), seed=seed, sub=2, rough=0.16,
               flat_bottom=0.55, tile=0.42, name="mossPatch")

    # A single simple collider follows the branch; the fungus shelves remain non-blocking detail.
    A.collide_box((2.0, 0.48, 0.52), loc=(0.0, 0.0, 0.20))
