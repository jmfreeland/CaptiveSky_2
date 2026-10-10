"""Read-only shortlist of possible fallen-wood hollow anchors in the saved Island.

Run as a UE 5.8.3 commandlet with the Island editor closed:
  UnrealEditor-Cmd <project.uproject> -ExecutePythonScript=<this file> -unattended -NoZen -NullRHI

The scan only loads the saved map and reports tag/name/mesh hints, transforms and bounds. It does not
modify or save actors, levels, Content, or resident data. A name match is only a visual shortlist;
the candidate still needs an in-editor inspection and navigation/capsule-clearance check.
"""

import re
import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
HOLLOW_TAGS = {"RatHollow", "FallenWoodHollow"}
WOOD_HINT = re.compile(r"fallen|driftwood|hollow|log|trunk|root|stump", re.I)
NON_HOLLOW_HINT = re.compile(r"bench|firewood|barrel|plank|fence", re.I)


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


def mesh_paths(actor):
    paths = []
    for component in mesh_components(actor):
        try:
            mesh = component.get_editor_property("static_mesh")
            if mesh:
                paths.append(mesh.get_path_name())
        except Exception:
            continue
    return paths


def describe(actor, paths, tags):
    location = actor.get_actor_location()
    try:
        origin, extent = actor.get_actor_bounds(False)
    except Exception:
        origin, extent = location, unreal.Vector(0.0, 0.0, 0.0)
    return "label={} actor={} class={} tags=[{}] location=({:.0f},{:.0f},{:.0f}) bounds_origin=({:.0f},{:.0f},{:.0f}) extent=({:.0f},{:.0f},{:.0f}) meshes=[{}]".format(
        actor.get_actor_label(), actor.get_name(), actor.get_class().get_name(),
        ",".join(sorted(tags)), location.x, location.y, location.z,
        origin.x, origin.y, origin.z, extent.x, extent.y, extent.z,
        ";".join(paths))


def candidate_kind(actor_label, actor_name, paths, tags):
    """Classify as an explicit marker or a weak name hint; neither proves suitability."""
    if HOLLOW_TAGS.intersection(tags):
        return "tagged"
    if not paths:
        return None
    identity = "{} {} {}".format(actor_label, actor_name, " ".join(paths))
    if WOOD_HINT.search(identity) and not NON_HOLLOW_HINT.search(identity):
        return "hint"
    return None


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[RatHollowAudit] Loaded {} actors from {}".format(len(actors), MAP_PATH))

    tagged = []
    hinted = []
    for actor in actors:
        tags = actor_tags(actor)
        paths = mesh_paths(actor)
        kind = candidate_kind(actor.get_actor_label(), actor.get_name(), paths, tags)
        if kind == "tagged":
            tagged.append((actor, paths, tags))
            continue
        if kind == "hint":
            hinted.append((actor, paths, tags))

    unreal.log("[RatHollowAudit] explicit_hollow_tags={} name_or_mesh_hints={}".format(len(tagged), len(hinted)))
    for actor, paths, tags in sorted(tagged, key=lambda row: row[0].get_actor_label()):
        unreal.log("[RatHollowAudit] TAGGED " + describe(actor, paths, tags))
    for actor, paths, tags in sorted(hinted, key=lambda row: row[0].get_actor_label()):
        unreal.log("[RatHollowAudit] HINT_ONLY " + describe(actor, paths, tags))
    if not tagged and not hinted:
        unreal.log("[RatHollowAudit] No matching name/tag/mesh hints; a visual map survey or authored hollow is still required.")
    unreal.log("[RatHollowAudit] COMPLETE; inspect candidates visually and verify nav/capsule clearance before spawning.")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.log_error("[RatHollowAudit] FAILED " + traceback.format_exc())
        raise
