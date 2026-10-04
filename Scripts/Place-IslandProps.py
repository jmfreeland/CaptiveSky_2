"""Places the generated props listed in Config/IslandProps.json into /Game/Maps/Island.

Every prop becomes a StaticMeshActor labelled Prop_<id>, tagged TripoProp and <id>, dropped onto the landscape (or
whatever solid surface is under it) at its x, y. Reruns first destroy every actor tagged TripoProp, so editing the
JSON and rerunning moves, adds or removes props without touching anything else in the map. The log reports the
surface hit, the ground slope under each prop's footprint and any prop whose mesh is missing.

Content/ is gitignored, so back up Content/Maps/Island.umap first (the map is saved at the end).

Run headless with the editor closed:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file> -unattended -NoZen -abslog=<abs log>
Set PLACE_PROPS_DRYRUN=1 to place and report without saving.
"""

import json
import math
import os
import traceback
import unreal

MAP = "/Game/Maps/Island"
TAG = "TripoProp"
ROOT = "/Game/Generated/Tripo"
CONFIG = os.path.join(unreal.Paths.project_dir(), "Config", "IslandProps.json")
SLOPE_WARN_CM = 60.0


def log(message):
    unreal.log("[PlaceProps] " + str(message))


def ground(world, x, y):
    """Highest solid surface under (x, y): returns (z, hit actor name) or (None, None)."""
    hit = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -2000.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if not hit:
        return None, None
    values = hit.to_tuple()
    point = next(v for v in values if isinstance(v, unreal.Vector))
    actor = next((v for v in values if isinstance(v, unreal.Actor)), None)
    return point.z, (actor.get_actor_label() if actor else "?")


def footprint_slope(world, x, y, radius):
    heights = []
    for dx, dy in ((0, 0), (radius, 0), (-radius, 0), (0, radius), (0, -radius)):
        z, _ = ground(world, x + dx, y + dy)
        if z is not None:
            heights.append(z)
    return max(heights) - min(heights) if heights else 0.0


def run():
    with open(CONFIG, "r", encoding="utf-8") as handle:
        props = json.load(handle)["props"]
    unreal.EditorLevelLibrary.load_level(MAP)
    world = unreal.EditorLevelLibrary.get_editor_world()

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if TAG in [str(t) for t in actor.tags]:
            unreal.EditorLevelLibrary.destroy_actor(actor)

    placed = 0
    for prop in props:
        name = prop["asset"]
        path = "{0}/{1}/{1}/StaticMeshes/{1}".format(ROOT, name)
        mesh = unreal.load_asset(path)
        if not mesh:
            log("SKIP {}: missing mesh {}".format(prop["id"], path))
            continue
        x, y = float(prop["x"]), float(prop["y"])
        z, surface = ground(world, x, y)
        if z is None:
            log("SKIP {}: nothing under ({:.0f}, {:.0f})".format(prop["id"], x, y))
            continue
        box = mesh.get_bounding_box()
        radius = max(box.max.x - box.min.x, box.max.y - box.min.y) / 2.0
        slope = footprint_slope(world, x, y, radius)
        z += float(prop.get("z_offset", 0.0))
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(prop.get("yaw", 0.0))))
        actor.set_actor_label("Prop_" + prop["id"])
        actor.tags = [TAG, prop["id"]]
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_static_mesh(mesh)
        placed += 1
        log("{:<22} {:<14} ({:.0f}, {:.0f}, {:.0f}) on {}  slope {:.0f} cm{}".format(
            prop["id"], name, x, y, z, surface, slope, "  <-- STEEP" if slope > SLOPE_WARN_CM else ""))

    log("placed {} of {}".format(placed, len(props)))
    if os.environ.get("PLACE_PROPS_DRYRUN") == "1":
        log("dry run: not saving")
        return
    log("saved level: {}".format(unreal.EditorLevelLibrary.save_current_level()))
    log("saved dirty packages: {}".format(unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)))


try:
    run()
except Exception:
    unreal.log_error("[PlaceProps] " + traceback.format_exc())
