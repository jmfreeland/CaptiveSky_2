"""Japanese-style stone lantern (ishidoro): base, shaft, platform, glowing firebox, flared roof, finial."""
NAME = "StoneLantern"
BAKE_SIZE = 2048


def build(A):
    A.cyl("stone", 0.34, 0.1, loc=(0, 0, 0.05), segs=8, tile=1.2, name="base")
    shaft = [(0, 0.1), (0.1, 0.1), (0.092, 0.2), (0.125, 0.3), (0.092, 0.4), (0.1, 0.56), (0, 0.56)]
    A.lathe("stone", shaft, segs=8, tile=1.2, name="shaft", smooth=False)
    A.cyl("stone", 0.29, 0.07, loc=(0, 0, 0.6), segs=8, tile=1.2, name="plat")
    A.cyl("stone", 0.25, 0.035, loc=(0, 0, 0.6525), segs=8, tile=1.2, name="lip")
    A.box("stone", (0.28, 0.28, 0.03), loc=(0, 0, 0.685), tile=1.2, bevel=0.004, name="floor")
    A.box("lanternglow", (0.15, 0.15, 0.2), loc=(0, 0, 0.8), name="glow")
    for sx in (-1, 1):
        for sy in (-1, 1):
            A.box("stone", (0.055, 0.055, 0.24), loc=(sx * 0.12, sy * 0.12, 0.8), tile=1.2, bevel=0.005, name="pillar")
    A.box("stone", (0.3, 0.3, 0.035), loc=(0, 0, 0.9175), tile=1.2, bevel=0.004, name="ceil")
    roof = [(0, 0.935), (0.4, 0.935), (0.41, 0.975), (0.24, 1.07), (0.1, 1.13), (0.05, 1.155), (0.065, 1.19), (0.05, 1.22), (0, 1.25)]
    A.lathe("stone", roof, segs=8, tile=1.2, name="roof", smooth=False)
    A.collide_cyl(0.36, 1.25, loc=(0, 0, 0.625), segs=8)
