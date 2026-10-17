"""Places the generated props listed in Config/IslandProps.json into /Game/Maps/Island.

Each prop becomes a StaticMeshActor labelled Prop_<id> and tagged GeneratedProp and <id>. Set `asset_path` to use any
imported Unreal StaticMesh; legacy entries with `asset` still resolve under /Game/Generated/Tripo. The default run is
preview-only. Set PLACE_PROPS_APPLY=1 to preflight every mesh and placement, then update only existing managed actors
whose IDs appear in the config and add missing IDs. All other generated and legacy actors are left untouched.

Content/ is gitignored, so back up Content/Maps/Island.umap first (the map is saved at the end).

Run headless with the editor closed:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file> -unattended -NoZen -abslog=<abs log>
Set PLACE_PROPS_DRYRUN=1 to force preview-only behavior, including when PLACE_PROPS_APPLY=1 is also set.
"""

import json
import math
import os
import traceback
import unreal

MAP = "/Game/Maps/Island"
TAG = "GeneratedProp"
LEGACY_TAG = "TripoProp"
ROOT = "/Game/Generated/Tripo"
CONFIG = os.path.join(unreal.Paths.project_dir(), "Config", "IslandProps.json")
SLOPE_WARN_CM = 60.0


def log(message):
    unreal.log("[PlaceProps] " + str(message))


def mesh_asset_path(prop, tripo_root=ROOT):
    """Use an explicit asset object path, or retain the legacy Tripo folder convention."""
    explicit = prop.get("asset_path")
    if explicit:
        return str(explicit)
    name = prop.get("asset")
    if not name:
        raise ValueError("{} needs either asset_path or asset".format(prop.get("id", "prop")))
    return "{0}/{1}/{1}/StaticMeshes/{1}".format(tripo_root, name)


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


def existing_props_by_id(actors, props):
    """Index only managed actors matching configured IDs; reject ambiguous matches before editing."""
    configured_ids = {str(prop["id"]) for prop in props}
    matches = {}
    for actor in actors:
        tags = {str(tag) for tag in actor.tags}
        if not ({TAG, LEGACY_TAG} & tags):
            continue
        actor_ids = configured_ids & tags
        if len(actor_ids) > 1:
            raise RuntimeError("managed actor {} matches multiple configured prop IDs".format(actor.get_actor_label()))
        if actor_ids:
            prop_id = next(iter(actor_ids))
            matches.setdefault(prop_id, []).append(actor)
    duplicates = [prop_id for prop_id, actors_for_id in matches.items() if len(actors_for_id) > 1]
    if duplicates:
        raise RuntimeError("multiple managed actors match configured ID(s): {}; no actors were changed".format(", ".join(sorted(duplicates))))
    return {prop_id: actors_for_id[0] for prop_id, actors_for_id in matches.items()}


def run():
    with open(CONFIG, "r", encoding="utf-8") as handle:
        props = json.load(handle)["props"]
    if any(not isinstance(prop.get("id"), str) or not prop["id"].strip() or prop["id"] != prop["id"].strip()
           for prop in props):
        raise ValueError("every prop needs a unique, non-empty string id")
    ids = [prop["id"] for prop in props]
    if len(ids) != len(set(ids)):
        raise ValueError("every prop needs a unique, non-empty id")
    unreal.EditorLevelLibrary.load_level(MAP)
    world = unreal.EditorLevelLibrary.get_editor_world()

    apply_requested = os.environ.get("PLACE_PROPS_APPLY") == "1"
    dry_run = os.environ.get("PLACE_PROPS_DRYRUN") == "1" or not apply_requested
    planned = []
    failures = []
    for prop in props:
        name = prop.get("asset", prop.get("id", "prop"))
        path = mesh_asset_path(prop)
        mesh = unreal.load_asset(path)
        if not mesh:
            failures.append("{}: missing mesh {}".format(prop["id"], path))
            continue
        x, y = float(prop["x"]), float(prop["y"])
        z, surface = ground(world, x, y)
        if z is None:
            failures.append("{}: nothing under ({:.0f}, {:.0f})".format(prop["id"], x, y))
            continue
        box = mesh.get_bounding_box()
        radius = max(box.max.x - box.min.x, box.max.y - box.min.y) / 2.0
        slope = footprint_slope(world, x, y, radius)
        z += float(prop.get("z_offset", 0.0))
        planned.append((prop, name, mesh, path, x, y, z, surface, slope))

    for failure in failures:
        log("PREFLIGHT FAILED " + failure)
    if failures and not dry_run:
        raise RuntimeError("Preflight found {} invalid prop(s); managed actors were left untouched".format(len(failures)))

    for prop, name, _mesh, path, x, y, z, surface, slope in planned:
        log("{} {:<14} {} ({:.0f}, {:.0f}, {:.0f}) on {}  slope {:.0f} cm{}".format(
            "WOULD PLACE" if dry_run else "READY", name, path, x, y, z, surface, slope,
            "  <-- STEEP" if slope > SLOPE_WARN_CM else ""))

    if dry_run:
        log("preview only: {} ready, {} failed; level unchanged. Set PLACE_PROPS_APPLY=1 to apply.".format(len(planned), len(failures)))
        return

    existing = existing_props_by_id(unreal.EditorLevelLibrary.get_all_level_actors(), props)
    for prop_id, actor in existing.items():
        if not isinstance(actor, unreal.StaticMeshActor) or not actor.static_mesh_component:
            raise RuntimeError("managed actor for {} is not a StaticMeshActor; no actors were changed".format(prop_id))

    placed = 0
    updated = 0
    for prop, _name, mesh, _path, x, y, z, surface, slope in planned:
        prop_id = str(prop["id"])
        rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(prop.get("yaw", 0.0)))
        actor = existing.get(prop_id)
        if actor:
            actor.set_actor_location_and_rotation(unreal.Vector(x, y, z), rotation, False, True)
            updated += 1
        else:
            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
                unreal.StaticMeshActor, unreal.Vector(x, y, z), rotation)
            placed += 1
        actor.set_actor_label("Prop_" + prop["id"])
        preserved_tags = [tag for tag in actor.tags if str(tag) not in {TAG, LEGACY_TAG, prop_id}]
        actor.tags = preserved_tags + [TAG, prop_id]
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_static_mesh(mesh)

    log("added {} and updated {} of {} configured props; unrelated managed actors were left untouched".format(placed, updated, len(props)))
    log("saved level: {}".format(unreal.EditorLevelLibrary.save_current_level()))


if __name__ == "__main__":
    try:
        run()
    except Exception:
        unreal.log_error("[PlaceProps] " + traceback.format_exc())
