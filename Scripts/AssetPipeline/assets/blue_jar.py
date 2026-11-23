"""Small blue-glazed stoneware jar with a hand-painted pale slip band."""
import math

import lib

NAME = "BlueJar"
BAKE_SIZE = 1024

# A deep, cool glaze with enough roughness to read as fired pottery in daylight.
lib.PLAIN["blue_jar_glaze"] = dict(BaseColor=[0.025, 0.11, 0.34], Roughness=0.24)
lib.PLAIN["blue_jar_slip"] = dict(BaseColor=[0.68, 0.75, 0.78], Roughness=0.42)


def build(A):
    # Foot, rounded belly, pulled shoulder, short neck and a hollow rolled rim.
    profile = [
        (0.0, 0.0), (0.064, 0.0), (0.071, 0.009), (0.073, 0.023),
        (0.086, 0.043), (0.103, 0.082), (0.108, 0.12), (0.101, 0.157),
        (0.084, 0.187), (0.064, 0.207), (0.058, 0.222), (0.058, 0.251),
        (0.066, 0.256), (0.067, 0.264), (0.061, 0.269), (0.048, 0.267),
        (0.046, 0.259), (0.047, 0.228), (0.054, 0.211), (0.075, 0.185),
        (0.091, 0.155), (0.096, 0.12), (0.091, 0.086), (0.076, 0.05),
        (0.058, 0.035), (0.0, 0.035),
    ]
    A.lathe("blue_jar_glaze", profile, segs=48, name="blue_jar_body", sharp=48)

    # Two quiet slip lines accent the neck; they follow its turned ceramic rim.
    A.torus("blue_jar_slip", 0.064, 0.0018, loc=(0.0, 0.0, 0.247),
            segs=48, rsegs=6, name="painted_neck_line")
    A.torus("blue_jar_slip", 0.071, 0.0015, loc=(0.0, 0.0, 0.026),
            segs=48, rsegs=6, name="painted_foot_line")

    # A small soft loop handle gives the vessel a readable hand-made silhouette.
    path = []
    for index in range(17):
        angle = math.radians(-105.0 + 210.0 * index / 16.0)
        path.append((0.078 + 0.025 * math.cos(angle), 0.0,
                     0.191 + 0.026 * math.sin(angle)))
    from mathutils import Vector
    A.tube("blue_jar_glaze", [Vector(point) for point in path], 0.006,
           segs=8, name="small_handle")

    A.collide_cyl(0.11, 0.27, loc=(0.0, 0.0, 0.135), segs=16)
