"""Asymmetric, wind-shaped bedrock arch with an open walking passage.

This is a separate review candidate; it does not replace the existing WindArch
export or alter the Island map. The goal is to make the landmark read as two
weathered outcrops joined by one fallen capstone, rather than regular courses of
stacked pebbles.
"""

NAME = "WindArchBedrock"
BAKE_SIZE = 1024


def build(A):
    # Keep moss near the sheltered feet and seams rather than carpeting the rock.
    A.moss = (1.65, 0.60)

    # Two distinct, slightly leaning monoliths preserve a broad central passage.
    A.rock("stone", (0.90, 0.77, 2.18), (-2.50, 0.00, 2.18),
           rot=(0.0, -3.0, -1.5), seed=241, sub=2, rough=0.25,
           flat_bottom=0.92, tile=1.35, name="westMonolith")
    A.rock("stone", (0.70, 0.82, 2.34), (2.55, 0.04, 2.34),
           rot=(1.5, 4.0, 2.0), seed=257, sub=2, rough=0.26,
           flat_bottom=0.90, tile=1.35, name="eastMonolith")

    # A single broad, off-level capstone carries the silhouette; direct contact
    # with the long monoliths avoids the obvious stacked-course tower effect.
    A.rock("stone", (2.48, 0.80, 0.55), (0.06, 0.00, 4.54),
           rot=(0.0, -2.5, 1.0), seed=293, sub=3, rough=0.18,
           flat_bottom=0.58, tile=1.35, name="windWornCap")

    # Low, half-buried talus stones make the piers feel seated in the ground.
    for i, (x, y, z, radii, seed) in enumerate((
        (-2.93, -0.23, 0.20, (0.74, 0.67, 0.28), 311),
        (-2.06, 0.28, 0.17, (0.57, 0.60, 0.22), 313),
        (2.89, 0.20, 0.21, (0.66, 0.70, 0.27), 317),
        (2.14, -0.31, 0.16, (0.61, 0.57, 0.21), 331),
    )):
        A.rock("stone", radii, (x, y, z), seed=seed, sub=2, rough=0.18,
               flat_bottom=0.84, tile=1.1, name="talus{:02d}".format(i))

    # Small irregular moss shelves collect at damp, shaded seams only.
    for i, (x, y, z, radii, seed) in enumerate((
        (-2.88, -0.32, 0.38, (0.32, 0.20, 0.075), 347),
        (-2.22, 0.26, 1.22, (0.23, 0.16, 0.060), 349),
        (2.73, -0.31, 0.55, (0.31, 0.18, 0.070), 353),
        (2.26, 0.28, 2.81, (0.21, 0.15, 0.055), 359),
        (-2.75, 0.26, 3.62, (0.25, 0.16, 0.055), 367),
    )):
        A.rock("moss", radii, (x, y, z), seed=seed, sub=2, rough=0.17,
               flat_bottom=0.30, tile=0.55, name="mossShelf{:02d}".format(i))

    # Coarse, hidden collision leaves the central route open and safe to traverse.
    A.collide_box((1.60, 1.55, 4.15), (-2.50, 0.00, 2.08))
    A.collide_box((1.48, 1.55, 3.90), (2.55, 0.04, 1.95))
    A.collide_box((6.30, 1.55, 1.05), (0.06, 0.00, 4.53))
