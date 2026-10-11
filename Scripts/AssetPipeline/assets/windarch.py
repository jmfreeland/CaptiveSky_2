"""A broad, weather-worn stone arch with an open passage and restrained moss."""

NAME = "WindArch"
BAKE_SIZE = 1024


def build(A):
    A.moss = (2.4, 0.56)

    # Two heavy, irregular piers leave a generous walk-through beneath the lintel.
    A.rock("stone", (0.82, 0.86, 2.12), (-2.48, 0.00, 2.12), seed=41,
           sub=3, rough=0.12, flat_bottom=0.86, tile=1.35, name="westPier")
    A.rock("stone", (0.78, 0.82, 2.04), (2.54, 0.04, 2.04), seed=52,
           sub=3, rough=0.13, flat_bottom=0.86, tile=1.35, name="eastPier")

    # Shoulder stones soften the join; the broad lintel reads as one wind-worn cap.
    A.rock("stone", (1.02, 0.98, 0.66), (-2.30, 0.00, 4.05), seed=63,
           sub=2, rough=0.12, flat_bottom=0.55, tile=1.35, name="westShoulder")
    A.rock("stone", (0.98, 0.94, 0.62), (2.30, 0.02, 4.01), seed=74,
           sub=2, rough=0.12, flat_bottom=0.55, tile=1.35, name="eastShoulder")
    A.rock("stone", (2.42, 0.86, 0.58), (0.02, 0.00, 4.57), seed=85,
           sub=3, rough=0.16, flat_bottom=0.52, tile=1.35, name="lintel")
    A.rock("stone", (0.72, 0.82, 0.46), (-0.12, 0.03, 4.77), seed=96,
           sub=2, rough=0.16, flat_bottom=0.62, tile=1.35, name="keystone")

    # Low broken skirts and a few moss patches make the base settle into the ground.
    for i, (x, y, z, radii, seed) in enumerate((
        (-2.78, -0.24, 0.32, (0.76, 0.68, 0.38), 107),
        (-2.18, 0.32, 0.22, (0.68, 0.58, 0.27), 118),
        (2.80, 0.21, 0.30, (0.72, 0.66, 0.34), 129),
        (2.20, -0.30, 0.22, (0.66, 0.58, 0.27), 140),
    )):
        A.rock("stone", radii, (x, y, z), seed=seed, sub=2, rough=0.14,
               flat_bottom=0.72, tile=1.1, name="foot{:02d}".format(i))

    for i, (x, y, z, radii) in enumerate((
        (-2.78, -0.30, 0.73, (0.38, 0.21, 0.10)),
        (-2.10, 0.23, 1.52, (0.28, 0.17, 0.09)),
        (2.63, -0.27, 0.95, (0.36, 0.18, 0.10)),
        (2.26, 0.20, 2.84, (0.25, 0.17, 0.08)),
        (-2.91, 0.13, 3.91, (0.30, 0.20, 0.08)),
        (2.08, -0.22, 3.79, (0.26, 0.18, 0.08)),
        (0.15, -0.39, 5.02, (0.36, 0.13, 0.07)),
    )):
        A.rock("moss", radii, (x, y, z), seed=151 + i, sub=2, rough=0.18,
               flat_bottom=0.35, tile=0.55, name="moss{:02d}".format(i))

    # Three boxes match the chunky stone masses while keeping the central route open.
    A.collide_box((1.55, 1.55, 4.20), (-2.48, 0.00, 2.10))
    A.collide_box((1.50, 1.50, 4.05), (2.54, 0.04, 2.025))
    A.collide_box((6.30, 1.55, 1.05), (0.02, 0.00, 4.55))
