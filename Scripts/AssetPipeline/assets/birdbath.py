"""Stone bird bath: turned pedestal + shallow basin, water disc, a couple of loose rocks."""
NAME = "BirdBath"
BAKE_SIZE = 2048


def build(A):
    prof = [(0, 0), (0.26, 0), (0.26, 0.04), (0.20, 0.07), (0.10, 0.13), (0.075, 0.2), (0.08, 0.4),
            (0.07, 0.55), (0.12, 0.62), (0.28, 0.68), (0.40, 0.74), (0.42, 0.81), (0.42, 0.83),
            (0.37, 0.83), (0.36, 0.79), (0.28, 0.76), (0.15, 0.745), (0, 0.74)]
    A.lathe("stone", prof, segs=40, tile=1.2, name="bath", sharp=30)
    A.rock("stone", (0.17, 0.13, 0.10), loc=(0.40, -0.22, 0.075), rot=(0, 0, 25), seed=3, tile=1.2, name="rock")
    A.rock("stone", (0.11, 0.10, 0.07), loc=(-0.33, 0.30, 0.05), rot=(0, 0, 70), seed=8, tile=1.2, name="rock")
    A.cyl("water", 0.33, 0.012, loc=(0, 0, 0.775), uv="box", segs=40, name="water")
    # a little stone bird perched on the rim
    A.rock("stone", (0.05, 0.075, 0.045), loc=(0.0, -0.395, 0.875), rot=(0, 0, 90), seed=5, sub=2, rough=0.04, flat_bottom=0.9, tile=1.2, name="bird")
    A.rock("stone", (0.03, 0.03, 0.03), loc=(0.0, -0.46, 0.925), seed=7, sub=1, rough=0.02, flat_bottom=0.9, tile=1.2, name="birdHead")
    A.cyl("stone", 0.01, 0.045, loc=(0.0, -0.5, 0.92), r2=0.003, rot=(90, 0, 0), segs=8, uv="box", tile=1.2, name="beak")
    A.box("stone", (0.04, 0.09, 0.012), loc=(0.0, -0.31, 0.885), rot=(-12, 0, 0), tile=1.2, name="birdTail")
    A.collide_cyl(0.28, 0.83, loc=(0, 0, 0.415))
