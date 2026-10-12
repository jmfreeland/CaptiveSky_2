"""A small, unevenly woven twig bowl with a dry grass lining for the Raven."""
import math

import bmesh
import lib
from mathutils import Vector


NAME = "RavenNest_WovenBowl"


def _point(radius, angle, z, drift=0.0):
    return Vector((
        (radius + drift) * math.cos(angle),
        (radius - drift * 0.35) * math.sin(angle),
        z,
    ))


def _twig(A, points, radius, seed, material="wood"):
    # Gently irregular centerlines and tapered broken ends avoid the machined,
    # identical-cane look of the previous engine-cylinder nest materializer.
    A.tube(material, points, radius, segs=8,
           taper=lambda t: 0.48 + 0.52 * min(t * 7.0, (1.0 - t) * 7.0, 1.0),
           name=f"weave{seed:02d}")


def build(A):
    # A nest is small enough that the shared, strongly grained plank texture
    # reads as striping. Use quiet matte colors so the weave silhouette carries it.
    lib.PLAIN["raven_nest_wood"] = {"BaseColor": [0.17, 0.105, 0.055], "Roughness": 0.94}
    lib.PLAIN["raven_nest_grass"] = {"BaseColor": [0.31, 0.25, 0.14], "Roughness": 0.97}

    # Five crossed foundation sticks leave the underside open and lightweight.
    for index, degrees in enumerate((0, 39, 83, 127, 166)):
        angle = math.radians(degrees)
        direction = Vector((math.cos(angle), math.sin(angle), 0.0))
        cross = Vector((-direction.y, direction.x, 0.0))
        center = Vector((0.0, 0.0, 0.026 + (index % 2) * 0.004))
        length = 0.39 + 0.025 * (index % 3)
        points = [center - direction * length * 0.5,
                  center - direction * length * 0.18 + cross * 0.006,
                  center + direction * length * 0.17 - cross * 0.005,
                  center + direction * length * 0.5]
        _twig(A, points, 0.0065, index)

    # Three broken, overlapping courses make a shallow cup rather than a stiff
    # perfect ring. Each segment is one continuous slender twig with visibly
    # tapered tips; the staggered ends suggest hand weaving.
    courses = ((0.145, 0.050, 0.012), (0.177, 0.094, 0.010), (0.211, 0.143, 0.009))
    segment_count = 9
    for course, (radius, height, twig_radius) in enumerate(courses):
        phase = course * math.radians(19.0)
        span = (2.0 * math.pi / segment_count) * 1.32
        for index in range(segment_count):
            start = 2.0 * math.pi * index / segment_count + phase
            points = []
            for step in range(6):
                t = step / 5.0
                angle = start + span * t
                # Alternate a slight up/down weave, and keep the rim a little
                # ragged instead of terminating every twig at one exact height.
                wave = math.sin(t * math.pi * 2.0 + index * 0.73 + course) * 0.007
                z = height + wave + 0.004 * math.sin(angle * 3.0 + index)
                drift = 0.004 * math.sin(index * 2.3 + course * 1.7)
                points.append(_point(radius, angle, z, drift))
            _twig(A, points, twig_radius, 10 + course * segment_count + index)

    # Fine grass is laid in a few irregular, low S-curves across the bowl floor.
    # This leaves a comfortable center rather than a radial starburst.
    for index in range(11):
        angle = math.radians((index * 37 + 11) % 180)
        direction = Vector((math.cos(angle), math.sin(angle), 0.0))
        across = Vector((-direction.y, direction.x, 0.0))
        center = across * ((index % 5 - 2) * 0.009) + direction * (((index * 3) % 7 - 3) * 0.006)
        half_length = 0.057 + 0.009 * (index % 4)
        z = 0.061 + 0.002 * (index % 3)
        points = []
        for step in range(5):
            t = step / 4.0
            point = center + direction * ((2.0 * t - 1.0) * half_length)
            point += across * (0.004 * math.sin(t * 2.0 * math.pi + index * 0.7))
            point.z += z + 0.004 * math.sin(t * math.pi)
            points.append(point)
        _twig(A, points, 0.0018, 50 + index, material="raven_nest_grass")

    # Two short strays break the silhouette asymmetrically without overwhelming
    # the nest with long, spoke-like twigs.
    for index, (angle, length, dz) in enumerate((
        (0.36, 0.245, 0.12), (3.71, 0.265, 0.15),
    )):
        start = _point(0.18, angle, dz)
        end = _point(length, angle + (0.10 if index % 2 else -0.08), dz + 0.035)
        middle = (start + end) * 0.5 + Vector((0.0, 0.0, 0.012))
        _twig(A, [start, middle, end], 0.0045, 70 + index, material="raven_nest_wood")

    # Apply the quiet palette to the main weave too.
    A.parts = [
        (obj, "raven_nest_wood" if material == "wood" else material)
        for obj, material in A.parts
    ]

    # Export with deterministic triangulation and a true ground-contact origin.
    # The pipeline joins these parts later, retaining the two shared materials.
    lowest = min(vertex.co.z for obj, _ in A.parts for vertex in obj.data.vertices)
    for obj, _ in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ngons = [face for face in bm.faces if len(face.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons, quad_method="BEAUTY", ngon_method="BEAUTY")
        for vertex in bm.verts:
            vertex.co.z -= lowest
        bm.to_mesh(obj.data)
        bm.free()
