"""Garden gate: two capped stone pillars with short wall stubs and a pair of braced plank gate leaves, one swung open."""
import math

NAME = "GardenGate"
BAKE_SIZE = 2048


def rot2(x, y, deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return x * c - y * s, x * s + y * c


def leaf(A, hx, side, angle):
    """Gate leaf hinged at x=hx (side = +1 for the left-hand leaf hinge, -1 right), swung by `angle` degrees about Z."""
    W, H = 0.74, 1.15
    z0 = 0.18

    def put(cx, cz, sx, sy, sz, name, rz=0.0, mat="wood", grain="Z"):
        ox, oy = rot2(side * cx, 0, angle)  # leaf extends toward the gate centre from the hinge
        A.box(mat, (sx, sy, sz), loc=(hx + ox, oy, z0 + cz), rot=(0, rz * -side, angle), grain=grain, bevel=0.004, name=name)

    # frame: stiles + rails + slats + diagonal brace
    put(0.03, H / 2, 0.06, 0.05, H, "stile")
    put(W - 0.03, H / 2, 0.06, 0.05, H, "stile")
    for cz in (0.1, H - 0.1):
        put(W / 2, cz, W, 0.05, 0.09, "rail", grain="X")
    for k in range(5):
        put(0.11 + k * 0.13, H / 2, 0.1, 0.03, H - 0.12, "slat")
    diag = math.degrees(math.atan2(H - 0.2, W - 0.1))
    put(W / 2, H / 2, math.hypot(W - 0.1, H - 0.2), 0.04, 0.07, "brace", rz=-diag, grain="X")
    # iron strap hinges
    for cz in (0.15, H - 0.15):
        put(0.2, cz, 0.4, 0.015, 0.045, "strap", mat="iron", grain="X")
    return


def build(A):
    A.moss = (1.2, 0.38)
    # pillars with cap + ball finial
    for sx in (-1, 1):
        x = sx * 1.0
        A.box("stone", (0.42, 0.42, 1.5), loc=(x, 0, 0.75), bevel=0.02, tile=1.2, name="pillar")
        A.box("stone", (0.52, 0.52, 0.08), loc=(x, 0, 1.54), bevel=0.015, tile=1.2, name="capslab")
        A.lathe("stone", [(0, 1.58), (0.28, 1.58), (0.2, 1.68), (0.1, 1.75), (0, 1.77)], loc=(x, 0, 0), segs=4, rot=(0, 0, 45), tile=1.2, smooth=False, name="cap")
        A.rock("stone", (0.09, 0.09, 0.09), loc=(x, 0, 1.82), seed=sx + 3, sub=2, rough=0.03, flat_bottom=0.9, tile=1.2, name="finial")
        # wall stubs
        A.box("stone", (1.1, 0.3, 0.75), loc=(sx * 1.78, 0, 0.375), bevel=0.02, tile=1.2, name="wall")
        A.box("stone", (1.18, 0.38, 0.08), loc=(sx * 1.78, 0, 0.79), bevel=0.015, tile=1.2, name="wallcap")
    # gate leaves between the pillars (inner faces at x=+-0.79): left hinged at -0.79 closed, right swung open 40 deg
    leaf(A, -0.79, 1, 0.0)
    leaf(A, 0.79, -1, 38.0)
    # latch ring on the closed leaf, stepping stone
    A.torus("iron", 0.04, 0.007, loc=(-0.12, -0.04, 0.78), rot=(90, 0, 0), segs=14, rsegs=5, name="latch")
    A.rock("stone", (0.3, 0.22, 0.05), loc=(0.0, -0.55, 0.03), seed=21, sub=2, rough=0.12, flat_bottom=0.5, tile=1.2, name="step")
    A.collide_box((0.5, 0.5, 1.9), loc=(-1.0, 0, 0.95))
    A.collide_box((0.5, 0.5, 1.9), loc=(1.0, 0, 0.95))
    A.collide_box((1.2, 0.4, 0.85), loc=(-1.78, 0, 0.42))
    A.collide_box((1.2, 0.4, 0.85), loc=(1.78, 0, 0.42))
    A.collide_box((0.78, 0.1, 1.3), loc=(-0.4, 0, 0.8))
