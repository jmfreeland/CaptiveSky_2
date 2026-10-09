"""Read-only shortlist of sizeable geometry near the Island's Raven roosts.

Run with UE 5.8's UnrealEditor-Cmd.exe and -ExecutePythonScript=<this file>.
The script loads the saved Island in an isolated process and does not move,
modify, or save any actor or map data. Bounding-box overlap is only a visual
shortlist; it does not prove canopy rain cover, perch support, or wind shelter.
"""

import math
import re
import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
ROOST_TAG = "RavenNestSite"
MIN_EXTENT_CM = 55.0
MAX_ROWS_PER_ROOST = 45
MAX_WORLD_EXTENT_CM = 5000.0
VEGETATION_HINT = re.compile(r"spruce|tree|plant|foliage|branch|canopy|mega|fern|shrub", re.I)


def actor_tags(actor):
    try:
        return {str(tag).split(".")[-1] for tag in actor.get_editor_property("tags")}
    except Exception:
        return set()


def mesh_components(actor):
    try:
        return actor.get_components_by_class(unreal.StaticMeshComponent)
    except Exception:
        return []


def bounds(actor):
    try:
        return actor.get_actor_bounds(False)
    except Exception:
        return actor.get_actor_location(), unreal.Vector(0.0, 0.0, 0.0)


def point_to_box_xy_distance(point, center, extent):
    dx = max(0.0, abs(point.x - center.x) - extent.x)
    dy = max(0.0, abs(point.y - center.y) - extent.y)
    return math.sqrt(dx * dx + dy * dy)


def describe_meshes(actor):
    paths = []
    for component in mesh_components(actor):
        try:
            mesh = component.get_editor_property("static_mesh")
            if mesh:
                paths.append(mesh.get_path_name())
        except Exception:
            continue
    return paths


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[RavenRoostShelterAudit] Loaded {} actors from {}".format(len(actors), MAP_PATH))

    roosts = []
    for actor in actors:
        tags = actor_tags(actor)
        if ROOST_TAG in tags and "RavenPerch" in tags:
            roosts.append((actor, actor.get_actor_location()))

    if not roosts:
        unreal.log_error("[RavenRoostShelterAudit] No actors carried both RavenNestSite and RavenPerch tags")
        return

    geometry = []
    for actor in actors:
        components = mesh_components(actor)
        if not components:
            continue
        center, extent = bounds(actor)
        if max(extent.x, extent.y, extent.z) < MIN_EXTENT_CM:
            continue
        if max(extent.x, extent.y, extent.z) > MAX_WORLD_EXTENT_CM:
            # Ignore ocean/water-info planes and other world-scale bounds.
            continue
        paths = describe_meshes(actor)
        if not paths:
            continue
        label = actor.get_actor_label()
        name = actor.get_name()
        tags = actor_tags(actor)
        geometry.append((actor, center, extent, paths, label, name, tags))

    for roost, location in sorted(roosts, key=lambda item: item[0].get_name()):
        label = roost.get_actor_label()
        unreal.log("[RavenRoostShelterAudit] ROOST label={} actor={} location=({:.1f},{:.1f},{:.1f})".format(
            label, roost.get_name(), location.x, location.y, location.z))
        rows = []
        for actor, center, extent, paths, actor_label, name, tags in geometry:
            if actor is roost:
                continue
            if actor_tags(actor).intersection({"Agent", "Raven"}) or actor.get_name().startswith("BP_"):
                continue
            distance_xy = point_to_box_xy_distance(location, center, extent)
            top_over_marker = center.z + extent.z - location.z
            # A possible overhead object must come reasonably close to the perch
            # horizontally and extend above it. Bounds are a shortlist, not proof
            # that the mesh actually covers the marker or blocks rain/wind.
            if distance_xy > 800.0 or top_over_marker < 100.0:
                continue
            horizontal_size = max(extent.x, extent.y)
            if horizontal_size < MIN_EXTENT_CM and extent.z < MIN_EXTENT_CM:
                continue
            identity = "{} {} {}".format(actor_label, name, " ".join(paths))
            botanical_hint = bool(VEGETATION_HINT.search(identity))
            rows.append((0 if botanical_hint else 1, distance_xy, -top_over_marker,
                         actor_label, name, center, extent, paths, tags, botanical_hint))

        rows.sort(key=lambda row: (row[0], row[1], row[2]))
        unreal.log("[RavenRoostShelterAudit] possible_overhead_bounds={} max_box_distance_cm=800".format(
            len(rows)))
        for row in rows[:MAX_ROWS_PER_ROOST]:
            _, distance_xy, neg_top, actor_label, name, center, extent, paths, tags, botanical_hint = row
            top_over_marker = -neg_top
            unreal.log(
                "[RavenRoostShelterAudit] {} d_box_xy={:.0f}cm top_minus_marker={:.0f}cm "
                "label={} actor={} center=({:.0f},{:.0f},{:.0f}) extent=({:.0f},{:.0f},{:.0f}) "
                "tags=[{}] meshes=[{}]".format(
                    "vegetation-name-hint" if botanical_hint else "other-geometry",
                    distance_xy, top_over_marker, actor_label, name,
                    center.x, center.y, center.z, extent.x, extent.y, extent.z,
                    ",".join(sorted(tags)), ";".join(paths)))


try:
    main()
except Exception:
    unreal.log_error("[RavenRoostShelterAudit] " + traceback.format_exc())
