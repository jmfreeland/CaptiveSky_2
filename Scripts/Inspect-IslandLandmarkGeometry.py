"""Read-only actor, static-mesh, and material inventory around Island landmarks."""

import traceback
import math
import unreal


LANDMARK_TAGS = ("ListeningStones", "WindArch", "TideglassPool", "Inn")
SEARCH_RADIUS_CM = 3500.0
MIN_BOUNDS_CM = 65.0
MAX_ROWS_PER_LANDMARK = 70


def actor_tags(actor):
    for getter in (
        lambda: actor.get_editor_property("tags"),
        lambda: actor.tags,
    ):
        try:
            return {str(tag) for tag in getter()}
        except Exception:
            pass
    return set()


def actor_bounds(actor):
    try:
        origin, extent = actor.get_actor_bounds(False)
        return origin, extent
    except Exception:
        return actor.get_actor_location(), unreal.Vector(0.0, 0.0, 0.0)


def mesh_components(actor):
    try:
        return actor.get_components_by_class(unreal.StaticMeshComponent)
    except Exception:
        return []


def component_description(component):
    mesh = component.static_mesh
    mesh_path = mesh.get_path_name() if mesh else "<no static mesh>"
    materials = []
    for index in range(component.get_num_materials()):
        material = component.get_material(index)
        materials.append(material.get_path_name() if material else "<none>")
    return "mesh={} materials={}".format(mesh_path, ";".join(materials) or "<none>")


def main():
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Island")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[LandmarkAudit] Loaded Island; actors={}".format(len(actors)))

    anchors = {}
    for actor in actors:
        tags = actor_tags(actor)
        for tag in LANDMARK_TAGS:
            if tag in tags:
                anchors.setdefault(tag, []).append(actor)
                location = actor.get_actor_location()
                unreal.log("[LandmarkAudit] marker={} actor={} label={} class={} location=({:.0f},{:.0f},{:.0f})".format(
                    tag, actor.get_name(), actor.get_actor_label(), actor.get_class().get_name(),
                    location.x, location.y, location.z))

    for tag in LANDMARK_TAGS:
        for anchor in anchors.get(tag, []):
            anchor_location = anchor.get_actor_location()
            candidates = []
            for actor in actors:
                if actor is anchor:
                    continue
                components = mesh_components(actor)
                if not components:
                    continue
                origin, extent = actor_bounds(actor)
                distance = math.sqrt(
                    (anchor_location.x - origin.x) ** 2 +
                    (anchor_location.y - origin.y) ** 2 +
                    (anchor_location.z - origin.z) ** 2)
                if distance > SEARCH_RADIUS_CM or max(extent.x, extent.y, extent.z) < MIN_BOUNDS_CM:
                    continue
                components_desc = " | ".join(component_description(component) for component in components[:4])
                candidates.append((distance, actor, origin, extent, components_desc))

            # Nearby large forms appear first, which makes blockout shapes easy to distinguish from scatter.
            candidates.sort(key=lambda item: (-(max(item[3].x, item[3].y, item[3].z)), item[0]))
            unreal.log("[LandmarkAudit] {} nearby_mesh_actors={} radius_cm={:.0f}".format(
                tag, len(candidates), SEARCH_RADIUS_CM))
            for distance, actor, origin, extent, components_desc in candidates[:MAX_ROWS_PER_LANDMARK]:
                tags = ",".join(sorted(actor_tags(actor)))
                unreal.log("[LandmarkAudit]   d={:.0f} label={} actor={} class={} tags=[{}] bounds=({:.0f},{:.0f},{:.0f}) {}".format(
                    distance, actor.get_actor_label(), actor.get_name(), actor.get_class().get_name(), tags,
                    extent.x, extent.y, extent.z, components_desc))


try:
    main()
except Exception:
    unreal.log_error("[LandmarkAudit] " + traceback.format_exc())
