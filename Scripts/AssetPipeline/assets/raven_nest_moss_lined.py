"""A woven Raven nest with a sparse, muted moss lining inside the lower bowl."""
import math
import os
import sys

import lib
from mathutils import Vector

# Reuse the established collisionless weave; this is a visual variation, not a
# new construction state or a change to the persistent in-world nest actor.
sys.path.insert(0, os.path.dirname(__file__))
import raven_nest


NAME = "RavenNest_WovenBowl_MossLined"
BAKE_SIZE = 512


def build(A):
    raven_nest.build(A)
    lib.PLAIN["raven_nest_moss"] = {
        "BaseColor": [0.145, 0.18, 0.09],
        "Roughness": 0.98,
    }

    # A few short, bent moss strands sit beneath the loose dry grass. Their
    # varied directions avoid a radial starburst or a flat, filled-in bowl.
    for index in range(18):
        angle = index * 2.399963 + 0.21 * math.sin(index * 1.7)
        center_radius = 0.012 + 0.043 * ((index * 7 % 17) / 16.0)
        center = Vector((center_radius * math.cos(angle), center_radius * math.sin(angle),
                         0.058 + 0.003 * math.sin(index * 2.3)))
        direction = Vector((math.cos(angle + 1.13), math.sin(angle + 1.13), 0.0))
        across = Vector((-direction.y, direction.x, 0.0))
        half_length = 0.023 + 0.009 * ((index * 5 % 11) / 10.0)
        bend = across * (0.004 * math.sin(index * 1.91))
        points = [
            center - direction * half_length,
            center + bend + Vector((0.0, 0.0, 0.003 + 0.001 * (index % 3))),
            center + direction * half_length,
        ]
        raven_nest._twig(A, points, 0.00125, 100 + index, material="raven_nest_moss")
