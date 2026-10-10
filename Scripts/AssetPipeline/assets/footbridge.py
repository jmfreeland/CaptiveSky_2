"""Arched plank footbridge with rope rails. Spans X; deck width 1.0 m."""
import math

from mathutils import Vector

NAME = "Footbridge"
HALF = 1.6


def deck_z(x, rise=0.5, base=0.14):
    return base + rise * (1 - (x / HALF) ** 2)


def build(A):
    n = 17
    w = 2 * HALF / n
    for i in range(n):  # deck planks follow the arc
        x = -HALF + w * (i + 0.5)
        ang = math.degrees(math.atan2(deck_z(x + 0.01) - deck_z(x - 0.01), 0.02))
        A.box("wood", (w - 0.012, 1.0, 0.055), loc=(x, 0, deck_z(x) - 0.03), rot=(0, -ang, 0), grain="Y", bevel=0.004, name="plank")
    for s in (-0.38, 0.38):  # curved stringers under the deck
        path = [Vector((x, s, deck_z(x) - 0.12)) for x in [(-HALF + 2 * HALF * t / 14) for t in range(15)]]
        A.tube("wood", path, 0.055, segs=8, name="stringer")
    for s in (-0.5, 0.5):  # posts + two rope rails per side
        for x in (-1.5, -0.75, 0.0, 0.75, 1.5):
            h = 0.95 if abs(x) > 1.0 else 0.8
            A.box("wood", (0.075, 0.075, h), loc=(x, s, deck_z(x) + h / 2 - 0.05), bevel=0.005, name="post")
        xs = [(-1.5 + 3.0 * t / 20) for t in range(21)]
        A.tube("rope", [Vector((x, s, deck_z(x) + 0.72)) for x in xs], 0.016, segs=6, name="rail")
        A.tube("rope", [Vector((x, s, deck_z(x) + 0.38)) for x in xs], 0.014, segs=6, name="rail_low")
    for x in (-1.62, 1.62):  # end sleepers on the ground
        A.box("wood", (0.18, 1.1, 0.1), loc=(x, 0, 0.05), grain="Y", bevel=0.006, name="sleeper")
    for i in range(7):  # joists
        x = -1.2 + 0.4 * i
        A.box("wood", (0.06, 0.9, 0.06), loc=(x, 0, deck_z(x) - 0.1), grain="Y", name="joist")
    for i in range(6):  # collision follows the arc in 6 slabs
        x0 = -HALF + (2 * HALF) * (i + 0.5) / 6
        slope = math.degrees(math.atan2(deck_z(x0 + 0.05) - deck_z(x0 - 0.05), 0.1))
        A.collide_box((0.62, 1.0, 0.12), loc=(x0, 0, deck_z(x0) - 0.03), rot=(0, -slope, 0))
