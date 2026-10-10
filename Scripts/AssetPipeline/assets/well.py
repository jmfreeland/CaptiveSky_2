"""Stone well: 54 jittered limestone blocks over a dark mortar core, wooden frame with gable roof,
windlass + crank, rope, hooped bucket, loose rocks. Re-authored from the hand-built original."""
NAME = "Well"
BAKE_SIZE = 4096


def build(A):
    R_IN, R_OUT, H = 0.55, 0.85, 0.9
    A.block_ring("stone", R_IN, R_OUT, courses=4, course_h=H / 4, per_course=13)
    A.lathe("mortar", [(R_IN + 0.04, 0.0), (R_OUT - 0.04, 0.0), (R_OUT - 0.04, 0.88), (R_IN + 0.04, 0.88), (R_IN + 0.04, 0.0)],
            segs=48, smooth=False, name="mortar")
    A.cyl("water", 0.535, 0.015, loc=(0, 0, 0.6), segs=40, uv="box", name="water")

    # frame: posts stand on the wall top, crossbeam, braces, gable roof with ridge cap
    for sx in (-0.7, 0.7):
        A.box("wood", (0.12, 0.12, 1.5), loc=(sx, 0, 1.63), bevel=0.008, name="post")
    A.box("wood", (0.12, 0.12, 1.55), loc=(0, 0, 2.3), rot=(0, 90, 0), bevel=0.006, name="crossbeam")
    for sx, rz in ((-0.52, 45), (0.52, -45)):
        A.box("wood", (0.07, 0.07, 0.42), loc=(sx, 0, 2.12), rot=(0, rz, 0), bevel=0.004, name="brace")
    for s in (1, -1):
        A.box("wood", (1.9, 0.07, 0.95), loc=(0, s * 0.37, 2.25), rot=(s * 52, 0, 0), grain="Z", bevel=0.004, name="roof")
    A.box("wood", (1.98, 0.14, 0.05), loc=(0, 0, 2.55), grain="X", name="ridge")

    # windlass, crank, rope, bucket
    A.cyl("wood", 0.07, 1.62, loc=(0.11, 0, 1.72), rot=(0, 90, 0), segs=20, name="windlass")
    A.cyl("rope", 0.1, 0.45, loc=(-0.1, 0, 1.72), rot=(0, 90, 0), segs=20, uv="box", name="coil")
    A.box("iron", (0.045, 0.045, 0.3), loc=(0.9, 0, 1.62), bevel=0.004, name="crank")
    A.cyl("wood", 0.03, 0.16, loc=(0.98, 0, 1.47), rot=(0, 90, 0), segs=12, name="handle")
    A.cyl("rope", 0.013, 0.52, loc=(0.15, 0, 1.36), segs=8, uv="box", name="rope")
    A.cyl("wood", 0.11, 0.22, loc=(0.15, 0, 0.98), r2=0.145, segs=20, name="bucket")
    A.torus("iron", 0.148, 0.009, loc=(0.15, 0, 1.055), segs=28, rsegs=6, name="hoop")
    A.torus("iron", 0.12, 0.009, loc=(0.15, 0, 0.91), segs=28, rsegs=6, name="hoop")

    # loose rocks at the base
    for (loc, rad, seed, rz) in (((1.12, -0.25, 0.07), (0.2, 0.15, 0.1), 3, 20), ((-0.98, 0.55, 0.06), (0.17, 0.2, 0.085), 8, -40),
                                 ((0.35, 1.05, 0.05), (0.14, 0.12, 0.07), 12, 70)):
        A.rock("stone", rad, loc=loc, rot=(0, 0, rz), seed=seed, tile=1.2, name="rock")
    A.collide_cyl(0.86, 0.9, loc=(0, 0, 0.45), segs=16)
