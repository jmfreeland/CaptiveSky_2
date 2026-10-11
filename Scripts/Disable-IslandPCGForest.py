"""Reversibly disable the confirmed submerged SimpleForest instance.

Run only after backing up Content/Maps/Island.umap and while no interactive
editor is open. This does not delete or edit /Game/PCG/PCG_World_Forest; it
deactivates its one Island component and hides its generated ISM outputs.
"""

import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
FOREST_LABEL = "PCG_ForestVolume_Island"
EXPECTED_GRAPH = "/Game/PCG/PCG_World_Forest.PCG_World_Forest"
EXPECTED_OUTPUT_COUNTS = {
    "ISM_PCG_Boulder_02_0": 6,
    "ISM_PCG_Seedling_01_0": 54918,
    "ISM_PCG_Tree_01_0": 1061,
    "ISM_PCG_Tree_02_0": 1107,
    "ISM_PCG_Tree_03_0": 1081,
}


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    forests = [actor for actor in actors if actor.get_actor_label() == FOREST_LABEL]
    if len(forests) != 1:
        raise RuntimeError("Expected exactly one '{}', found {}. Nothing saved.".format(FOREST_LABEL, len(forests)))
    forest = forests[0]

    components = forest.get_components_by_class(unreal.ActorComponent)
    pcg_components = [component for component in components if component.get_class().get_name() == "PCGComponent"]
    if len(pcg_components) != 1:
        raise RuntimeError("Expected exactly one PCGComponent, found {}. Nothing saved.".format(len(pcg_components)))
    pcg = pcg_components[0]
    graph = pcg.get_graph()
    graph_path = graph.get_path_name() if graph else "none"
    if graph_path != EXPECTED_GRAPH:
        raise RuntimeError("Unexpected PCG graph '{}'. Nothing saved.".format(graph_path))

    outputs = [component for component in components
               if isinstance(component, unreal.InstancedStaticMeshComponent)
               and component.get_name().startswith("ISM_PCG_")]
    found_counts = {component.get_name(): component.get_instance_count() for component in outputs}
    if found_counts != EXPECTED_OUTPUT_COUNTS:
        raise RuntimeError("PCG output inventory changed: {}. Nothing saved.".format(found_counts))

    activated_before = pcg.get_editor_property("activated")
    unreal.log("[PCGForestDisable] target={} graph={} activated_before={} outputs={}".format(
        FOREST_LABEL, graph_path, activated_before, found_counts))

    pcg.set_editor_property("activated", False)
    for component in outputs:
        component.set_visibility(False, True)

    activated_after = pcg.get_editor_property("activated")
    output_state = {
        component.get_name(): {
            "visible": component.get_editor_property("visible"),
            "collision_enabled": component.is_collision_enabled(),
            "collision_profile": str(component.get_editor_property("body_instance").get_editor_property("collision_profile_name")),
            "instances": component.get_instance_count(),
        }
        for component in outputs
    }
    if activated_after or any(state["visible"] for state in output_state.values()):
        raise RuntimeError("Disable verification failed before save: {}".format(output_state))

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save {} after disabling the isolated PCG output.".format(MAP_PATH))
    unreal.log("[PCGForestDisable] saved={} activated_after={} outputs={}".format(
        MAP_PATH, activated_after, output_state))
    unreal.log("[PCGForestDisable] COMPLETE; graph asset preserved; output instances retained but hidden; existing collision profiles unchanged.")


try:
    main()
except Exception:
    unreal.log_error("[PCGForestDisable] FAILED " + traceback.format_exc())
    raise
