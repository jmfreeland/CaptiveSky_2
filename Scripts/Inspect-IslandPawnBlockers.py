"""Read-only audit of saved Island actors implicated in grounded Pawn sweeps."""

import traceback
import unreal


MESH_FRAGMENTS = (
    "grass_01_03_mesh",
    "LanternPost",
    "/SM_Rock.",
)
KNOWN_ACTOR_NAMES = {
    "StaticMeshActor_18",
    "StaticMeshActor_35",
    "StaticMeshActor_37",
    "StaticMeshActor_137",
}


def safe_property(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return "<unavailable>"


def safe_method(obj, name):
    try:
        return getattr(obj, name)()
    except Exception:
        return "<unavailable>"


def pawn_response(component):
    try:
        return component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)
    except Exception:
        return safe_property(component, "collision_responses")


def actor_tags(actor):
    try:
        return ",".join(sorted(str(tag) for tag in actor.get_editor_property("tags"))) or "none"
    except Exception:
        return "<unavailable>"


def main():
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Island")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[PawnBlockerAudit] Read-only Island audit; actors={}".format(len(actors)))

    matches = 0
    for actor in actors:
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        descriptions = []
        matched = actor.get_name() in KNOWN_ACTOR_NAMES
        for component in components:
            mesh = component.static_mesh
            mesh_path = mesh.get_path_name() if mesh else "<no static mesh>"
            if any(fragment in mesh_path for fragment in MESH_FRAGMENTS):
                matched = True
            descriptions.append((component, mesh_path))
        if not matched:
            continue

        location = actor.get_actor_location()
        origin, extent = actor.get_actor_bounds(False)
        unreal.log(
            "[PawnBlockerAudit] actor={} label={} class={} tags=[{}] location=({:.1f},{:.1f},{:.1f}) bounds_origin=({:.1f},{:.1f},{:.1f}) extent=({:.1f},{:.1f},{:.1f})".format(
                actor.get_name(), actor.get_actor_label(), actor.get_class().get_name(), actor_tags(actor),
                location.x, location.y, location.z, origin.x, origin.y, origin.z,
                extent.x, extent.y, extent.z))
        for component, mesh_path in descriptions:
            unreal.log(
                "[PawnBlockerAudit]   component={} mesh={} profile={} collision={} object_type={} pawn_response={} affects_nav={} overlap_events={}".format(
                    component.get_name(), mesh_path,
                    safe_method(component, "get_collision_profile_name"),
                    safe_method(component, "get_collision_enabled"),
                    safe_method(component, "get_collision_object_type"), pawn_response(component),
                    safe_property(component, "can_ever_affect_navigation"),
                    safe_property(component, "generate_overlap_events")))
        matches += 1

    unreal.log("[PawnBlockerAudit] Complete; matching actors={}; level was not saved.".format(matches))


try:
    main()
except Exception:
    unreal.log_error("[PawnBlockerAudit] " + traceback.format_exc())
