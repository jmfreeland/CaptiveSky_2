"""Read-only audit of the legacy PCG forest volume in the saved Island.

Run with UE 5.8's UnrealEditor-Cmd.exe while no interactive editor is open:
  UnrealEditor-Cmd.exe CaptiveSky_2.uproject -ExecutePythonScript="<project-root>/Scripts/Inspect-IslandPCGForest.py" -unattended -NoZen -NullRHI

This loads but never saves /Game/Maps/Island. It reports the forest PCG volume's
world bounds, graph assignment, and the ocean plane's bounds for comparison.
"""

import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
FOREST_LABEL = "PCG_ForestVolume_Island"
OCEAN_LABEL = "OceanPlane"
LANDSCAPE_LABEL = "Landscape"


def describe_bounds(actor):
    origin, extent = actor.get_actor_bounds(False)
    minimum = unreal.Vector(origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
    maximum = unreal.Vector(origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
    return origin, extent, minimum, maximum


def transform_position(transform, point):
    scale = transform.scale3d
    scaled = unreal.Vector(point.x * scale.x, point.y * scale.y, point.z * scale.z)
    rotation = transform.rotation
    qv = unreal.Vector(rotation.x, rotation.y, rotation.z)
    cross_a = unreal.Vector(qv.y * scaled.z - qv.z * scaled.y,
                            qv.z * scaled.x - qv.x * scaled.z,
                            qv.x * scaled.y - qv.y * scaled.x)
    cross_b = unreal.Vector(qv.y * cross_a.z - qv.z * cross_a.y,
                            qv.z * cross_a.x - qv.x * cross_a.z,
                            qv.x * cross_a.y - qv.y * cross_a.x)
    rotated = scaled + cross_a * (2.0 * rotation.w) + cross_b * 2.0
    return transform.translation + rotated


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[PCGForestAudit] loaded map={} actors={}".format(MAP_PATH, len(actors)))

    forest = next((actor for actor in actors if actor.get_actor_label() == FOREST_LABEL), None)
    ocean = next((actor for actor in actors if actor.get_actor_label() == OCEAN_LABEL), None)
    landscape = next((actor for actor in actors if actor.get_actor_label() == LANDSCAPE_LABEL), None)
    if forest is None:
        unreal.log_error("[PCGForestAudit] Forest actor '{}' not found; no changes made.".format(FOREST_LABEL))
        return

    origin, extent, minimum, maximum = describe_bounds(forest)
    unreal.log("[PCGForestAudit] forest label={} class={} location={} bounds_origin={} extent={} min={} max={}".format(
        forest.get_actor_label(), forest.get_class().get_name(), forest.get_actor_location(),
        origin, extent, minimum, maximum))

    forest_graph = None
    actor_components = forest.get_components_by_class(unreal.ActorComponent)
    unreal.log("[PCGForestAudit] forest_component_count={}".format(len(actor_components)))
    for component in actor_components:
        class_name = component.get_class().get_name()
        unreal.log("[PCGForestAudit] forest_component={} class={}".format(component.get_name(), class_name))
        if isinstance(component, unreal.InstancedStaticMeshComponent):
            count = component.get_instance_count()
            mesh = component.get_editor_property("static_mesh")
            mesh_path = mesh.get_path_name() if mesh else "none"
            mesh_bounds = mesh.get_bounds() if mesh else None
            mesh_height = mesh_bounds.box_extent.z * 2.0 if mesh_bounds else 0.0
            indices = sorted(set([0, count // 4, count // 2, (3 * count) // 4, count - 1])) if count else []
            sample_z = []
            for index in indices:
                result = component.get_instance_transform(index, True)
                transform = result[-1] if isinstance(result, tuple) else result
                sample_z.append("{}:{:.1f}".format(index, transform.translation.z))
            unreal.log("[PCGForestAudit] instances component={} mesh={} count={} mesh_height_cm={:.1f} sampled_world_root_z_cm=[{}]".format(
                component.get_name(), mesh_path, count, mesh_height, ",".join(sample_z)))
            if mesh_bounds and "Tree" in component.get_name() and ocean is not None:
                local_center = mesh_bounds.origin
                local_extent = mesh_bounds.box_extent
                local_corners = [
                    unreal.Vector(local_center.x + sx * local_extent.x,
                                  local_center.y + sy * local_extent.y,
                                  local_center.z + sz * local_extent.z)
                    for sx in (-1.0, 1.0) for sy in (-1.0, 1.0) for sz in (-1.0, 1.0)
                ]
                water_z = ocean.get_actor_location().z
                crossing = submerged = above = 0
                root_min = float("inf")
                root_max = float("-inf")
                for index in range(count):
                    result = component.get_instance_transform(index, True)
                    transform = result[-1] if isinstance(result, tuple) else result
                    root_z = transform.translation.z
                    root_min = min(root_min, root_z)
                    root_max = max(root_max, root_z)
                    world_z = [transform_position(transform, corner).z for corner in local_corners]
                    if min(world_z) < water_z < max(world_z):
                        crossing += 1
                    elif max(world_z) <= water_z:
                        submerged += 1
                    else:
                        above += 1
                unreal.log("[PCGForestAudit] tree_waterline component={} roots_z_cm={:.1f}..{:.1f} water_z_cm={:.1f} mesh_bounds crossing={} submerged={} above={}".format(
                    component.get_name(), root_min, root_max, water_z, crossing, submerged, above))
        if "PCG" not in class_name:
            continue
        graph_path = "unavailable"
        try:
            graph = component.get_graph()
            if graph:
                graph_path = graph.get_path_name()
                forest_graph = graph
        except Exception as error:
            graph_path = "unavailable ({})".format(error)
        try:
            activated = component.get_editor_property("activated")
        except Exception as error:
            activated = "unavailable ({})".format(error)
        try:
            generated = component.get_editor_property("generated")
        except Exception as error:
            generated = "unavailable ({})".format(error)
        unreal.log("[PCGForestAudit] component={} class={} graph={} activated={} generated={}".format(
            component.get_name(), class_name, graph_path, activated, generated))

    if forest_graph is not None:
        try:
            nodes = forest_graph.get_editor_property("nodes")
            unreal.log("[PCGForestAudit] graph_nodes={}".format(len(nodes)))
            for node in nodes:
                settings = node.get_settings()
                settings_class = settings.get_class().get_name() if settings else "none"
                unreal.log("[PCGForestAudit] graph_node={} settings={}".format(node.get_name(), settings_class))
                for pin in node.get_editor_property("output_pins"):
                    for edge in pin.get_editor_property("edges"):
                        downstream = edge.get_output_node()
                        unreal.log("[PCGForestAudit] graph_edge={} -> {}".format(
                            node.get_name(), downstream.get_name() if downstream else "none"))
        except Exception as error:
            unreal.log_warning("[PCGForestAudit] graph node inspection unavailable: {}".format(error))

    if ocean is not None:
        ocean_origin, ocean_extent, ocean_min, ocean_max = describe_bounds(ocean)
        unreal.log("[PCGForestAudit] ocean label={} location={} bounds_origin={} extent={} min={} max={}".format(
            ocean.get_actor_label(), ocean.get_actor_location(), ocean_origin, ocean_extent, ocean_min, ocean_max))
        unreal.log("[PCGForestAudit] forest_bounds_cross_ocean_z={}".format(
            minimum.z <= ocean_origin.z <= maximum.z))
        unreal.log("[PCGForestAudit] forest_xy_inside_ocean_bounds={}".format(
            ocean_min.x <= origin.x <= ocean_max.x and ocean_min.y <= origin.y <= ocean_max.y))
    else:
        unreal.log_warning("[PCGForestAudit] OceanPlane actor not found; waterline comparison unavailable.")

    if landscape is not None:
        land_origin, land_extent, land_min, land_max = describe_bounds(landscape)
        overlaps_land_xy = (minimum.x <= land_max.x and maximum.x >= land_min.x
                             and minimum.y <= land_max.y and maximum.y >= land_min.y)
        unreal.log("[PCGForestAudit] landscape label={} bounds_origin={} extent={} min={} max={}".format(
            landscape.get_actor_label(), land_origin, land_extent, land_min, land_max))
        unreal.log("[PCGForestAudit] forest_overlaps_landscape_xy={}".format(overlaps_land_xy))
    else:
        unreal.log_warning("[PCGForestAudit] Landscape actor not found; land overlap unavailable.")

    # Separate a real terrain hit from the huge ocean plane and report which
    # actor supports the sampled PCG footprint. These are diagnostic traces only.
    sample_points = [
        (origin.x, origin.y, "center"),
        (minimum.x + 1000.0, minimum.y + 1000.0, "southwest"),
        (minimum.x + 1000.0, maximum.y - 1000.0, "northwest"),
        (maximum.x - 1000.0, minimum.y + 1000.0, "southeast"),
        (maximum.x - 1000.0, maximum.y - 1000.0, "northeast"),
    ]
    world = unreal.EditorLevelLibrary.get_editor_world()
    ignored_forest = [forest]
    ignored_forest_and_ocean = [forest] + ([ocean] if ocean is not None else [])
    for x, y, sample_name in sample_points:
        visible_hit = unreal.SystemLibrary.line_trace_single(
            world, unreal.Vector(x, y, 60000.0), unreal.Vector(x, y, -20000.0),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignored_forest, unreal.DrawDebugTrace.NONE, True)
        ground_hit = unreal.SystemLibrary.line_trace_single(
            world, unreal.Vector(x, y, 60000.0), unreal.Vector(x, y, -20000.0),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignored_forest_and_ocean, unreal.DrawDebugTrace.NONE, True)

        def hit_summary(hit):
            if hit is None:
                return "none"
            hit_tuple = hit.to_tuple() if hasattr(hit, "to_tuple") else ()
            hit_actor = next((value for value in hit_tuple if hasattr(value, "get_actor_label")), None)
            hit_label = hit_actor.get_actor_label() if hit_actor else "unknown"
            hit_class = hit_actor.get_class().get_name() if hit_actor else "unknown"
            point = hit.impact_point if hasattr(hit, "impact_point") else (hit_tuple[5] if len(hit_tuple) > 5 else "unavailable")
            return "{}:{}@{}".format(hit_label, hit_class, point)

        unreal.log("[PCGForestAudit] sample={} xy=({:.0f},{:.0f}) first_surface={} ground_beneath_ocean={}".format(
            sample_name, x, y, hit_summary(visible_hit), hit_summary(ground_hit)))

    unreal.log("[PCGForestAudit] COMPLETE; read-only, map not saved.")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.log_error("[PCGForestAudit] FAILED " + traceback.format_exc())
        raise
