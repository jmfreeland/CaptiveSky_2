"""Replaces the Island's flat ocean plane with a Water-plugin ocean (Gerstner waves, shoreline depth, underwater view).

Authors into /Game/Maps/Island:
  - one WaterZone centred on the island at sea level, covering the old ocean plane's footprint;
  - one WaterBodyOcean filling that zone (the landscape hides it wherever land is above sea level);
  - the old `OceanPlane` actor is kept but made invisible, so it can be turned back on (revert) and the
    IslandOceanSubsystem / viewpoint harness still find it.
The ocean has no collision (like the plane before it, and so residents never switch to swimming) and does not
carve the landscape. Rebuilds its own zone and ocean on every run (tagged IslandWaterBody) and nothing else.

Content/ is gitignored, so back up Content/Maps/Island.umap and Content/__ExternalActors__/Maps/Island first.

Run in the full editor so the water-info texture renders, and let it keep ticking until it saves and quits:
  UnrealEditor.exe <uproject> -ExecCmds="py <this file>" -unattended -NoZen -abslog=<log>
Set WATER_BODY_DRYRUN=1 to build without saving or quitting.
"""

import os
import traceback
import unreal

MAP = "/Game/Maps/Island"
SEA_LEVEL_CM = 940.0
ZONE_EXTENT_CM = 614400.0  # the old plane: 2400 x the 256 cm plane mesh
WATER_INFO_RESOLUTION = 4096
TAG = "IslandWaterBody"
PLANE_NAME = "OceanPlane"
SETTLE_SECONDS = 25.0

state = {"stage": 0, "start": None, "handle": None, "zone": None, "ocean": None}


def log(message):
    unreal.log("[WaterBody] " + str(message))


def tagged(actor):
    return TAG in [str(t) for t in actor.tags]


def build():
    unreal.EditorLevelLibrary.load_level(MAP)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()

    for actor in actors:
        if tagged(actor) and isinstance(actor, (unreal.WaterZone, unreal.WaterBodyOcean)):
            log("Removing previous {}".format(actor.get_actor_label()))
            unreal.EditorLevelLibrary.destroy_actor(actor)

    origin = unreal.Vector(0.0, 0.0, SEA_LEVEL_CM)
    zone = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.WaterZone, origin)
    zone.set_actor_label("IslandWaterZone")
    zone.tags = [TAG]
    zone.set_editor_property("zone_extent", unreal.Vector2D(ZONE_EXTENT_CM, ZONE_EXTENT_CM))
    zone.set_editor_property("render_target_resolution", unreal.IntPoint(WATER_INFO_RESOLUTION, WATER_INFO_RESOLUTION))

    ocean = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.WaterBodyOcean, origin)
    ocean.set_actor_label("IslandOcean")
    ocean.tags = [TAG]
    body = ocean.water_body_component
    body.set_editor_property("ocean_extents", unreal.Vector2D(ZONE_EXTENT_CM, ZONE_EXTENT_CM))
    body.set_editor_property("affects_landscape", False)
    body.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    for actor in actors:
        if actor.get_actor_label() == PLANE_NAME and isinstance(actor, unreal.StaticMeshActor):
            actor.static_mesh_component.set_visibility(False)
            log("{} hidden".format(PLANE_NAME))

    state["zone"], state["ocean"] = zone, ocean
    log("zone extent {} resolution {} bounds {}".format(zone.get_editor_property("zone_extent"), zone.get_editor_property("render_target_resolution"), zone.get_actor_bounds(False)))
    log("ocean material {} extents {}".format(body.get_editor_property("water_material").get_path_name(), body.get_editor_property("ocean_extents")))
    waves = ocean.get_editor_property("water_waves")
    log("waves reference {} members {}".format(waves, [m for m in dir(waves) if not m.startswith("_") and m not in dir(unreal.Object)]))


def on_tick(delta):
    import time
    if state["start"] is None:
        state["start"] = time.time()
    elapsed = time.time() - state["start"]
    if state["stage"] == 0 and elapsed > SETTLE_SECONDS:
        state["stage"] = 1
        zone = state["zone"]
        log("info texture slices {}".format(zone.get_editor_property("water_info_texture_array_num_slices")))
        if os.environ.get("WATER_BODY_DRYRUN") == "1":
            log("dry run: not saving")
        else:
            log("saved level: {}".format(unreal.EditorLevelLibrary.save_current_level()))
            log("saved dirty packages: {}".format(unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)))
    elif state["stage"] == 1 and elapsed > SETTLE_SECONDS + 3.0:
        state["stage"] = 2
        unreal.unregister_slate_post_tick_callback(state["handle"])
        if os.environ.get("WATER_BODY_DRYRUN") != "1":
            unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")


try:
    build()
    state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
except Exception:
    unreal.log_error("[WaterBody] " + traceback.format_exc())
