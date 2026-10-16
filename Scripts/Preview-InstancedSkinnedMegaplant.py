"""Compare one transient Megaplant through instanced and ordinary skeletal paths.

This is a disposable visual/API probe. It loads the Island as a lighting and
scale reference, adds exactly one transient instance, captures one frame, and
never saves the map or any asset. Run with UnrealEditor-Cmd.exe, the project
uproject, -RenderOffscreen, and -ExecutePythonScript=<absolute script path>.
CAPTIVESKY_SKINNED_PREVIEW_ASSET and CAPTIVESKY_SKINNED_PREVIEW_OUTPUT may
override the default Beech A asset and project-relative output directory.
CAPTIVESKY_SKINNED_PREVIEW_HOUR selects a transient capture hour (default noon).
The default output folder and comparison filenames are derived from the asset
name so alternate species and variants cannot overwrite or masquerade as Beech.
"""
import json
import os
import re
import traceback

import unreal


PREFIX = "[InstancedSkinnedMegaplant] "
MAP = "/Game/Maps/Island"
ASSET = os.environ.get(
    "CAPTIVESKY_SKINNED_PREVIEW_ASSET",
    "/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_A.Tree_European_Beech_01_A",
)
ASSET_PACKAGE_NAME = os.path.basename(ASSET.rsplit(".", 1)[0])
ASSET_LABEL = re.sub(r"[^A-Za-z0-9_-]+", "_", ASSET_PACKAGE_NAME).strip("_") or "Candidate"
TARGET_LABEL = os.environ.get("CAPTIVESKY_SKINNED_PREVIEW_TARGET", "Roost_West")
PREVIEW_HOUR = float(os.environ.get("CAPTIVESKY_SKINNED_PREVIEW_HOUR", "12.0"))
if not 0.0 <= PREVIEW_HOUR < 24.0:
    raise ValueError("CAPTIVESKY_SKINNED_PREVIEW_HOUR must be in [0, 24)")
OUTPUT_RELATIVE = os.environ.get(
    "CAPTIVESKY_SKINNED_PREVIEW_OUTPUT",
    "Saved/Playtests/Codex_InstancedSkinned_" + ASSET_LABEL,
)
OUTPUT_FILENAME = "InstancedSkinned_{}.png".format(ASSET_LABEL)
STANDARD_OUTPUT_FILENAME = "SkeletalMesh_{}.png".format(ASSET_LABEL)
FOLIAGE_TINT = tuple(float(value) for value in os.environ.get(
    "CAPTIVESKY_SKINNED_PREVIEW_FOLIAGE_TINT", "0.48,0.65,0.35,1.0"
).split(","))
if len(FOLIAGE_TINT) != 4:
    raise ValueError("CAPTIVESKY_SKINNED_PREVIEW_FOLIAGE_TINT must contain RGBA values")
FOV_ANGLE = 48.0
PREVIEW_STATE = {}


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def components(vector):
    return [round(float(getattr(vector, axis)), 2) for axis in ("x", "y", "z")]


def material_parameters(material):
    result = {"path": material.get_path_name(), "class": material.get_class().get_name()}
    try:
        parent = material.get_editor_property("parent")
        result["parent"] = parent.get_path_name() if parent else None
    except Exception as error:
        result["parent_error"] = str(error)

    for property_name, value_property in (
        ("texture_parameter_values", "parameter_value"),
        ("scalar_parameter_values", "parameter_value"),
        ("vector_parameter_values", "parameter_value"),
    ):
        entries = []
        try:
            values = material.get_editor_property(property_name)
        except Exception as error:
            result[property_name + "_error"] = str(error)
            continue
        for entry in values:
            try:
                parameter_info = entry.get_editor_property("parameter_info")
                parameter_name = str(parameter_info.get_editor_property("name"))
                value = entry.get_editor_property(value_property)
                if hasattr(value, "get_path_name"):
                    value = value.get_path_name() if value else None
                elif all(hasattr(value, axis) for axis in ("r", "g", "b", "a")):
                    value = [round(float(getattr(value, axis)), 4) for axis in ("r", "g", "b", "a")]
                elif isinstance(value, (int, float, bool, str)) or value is None:
                    value = value
                else:
                    value = str(value)
                entries.append({"name": parameter_name, "value": value})
            except Exception as error:
                entries.append({"error": str(error)})
        result[property_name] = entries

    return result


def on_preview_tick(delta_seconds):
    state = PREVIEW_STATE
    state["ticks"] += 1
    try:
        if state["ticks"] == 3:
            for capture in state["captures"]:
                capture.capture_scene()
            log("instanced and ordinary skeletal captures requested at frame {}".format(
                unreal.SystemLibrary.get_frame_count()
            ))
        elif state["ticks"] >= 5:
            for filename, render_target in state["outputs"]:
                unreal.RenderingLibrary.export_render_target(
                    state["world"], render_target, state["output_directory"], filename
                )
            log("REPORT " + json.dumps(state["report"], sort_keys=True))
            log("COMPLETE")
            unreal.unregister_slate_post_tick_callback(state["handle"])
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()
    except Exception:
        unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
        if state.get("handle") is not None:
            unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


def main():
    require(unreal.EditorLoadingAndSavingUtils.load_map(MAP), "Could not load Island review map")
    world = require(
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
        "Editor world is unavailable",
    )
    actor_tools = require(unreal.get_editor_subsystem(unreal.EditorActorSubsystem),
                          "Editor Actor Subsystem is unavailable")
    island_clock = next(
        (actor for actor in actor_tools.get_all_level_actors()
         if actor.get_class().get_name() == "IslandDayNight"),
        None,
    )
    require(island_clock, "Could not find IslandDayNight clock actor")
    island_clock.set_editor_property("start_hour", PREVIEW_HOUR)
    capture_hour = float(island_clock.get_editor_property("current_hour"))
    require(abs(capture_hour - PREVIEW_HOUR) < 0.05,
            "Transient clock override did not apply: expected {}, got {}".format(
                PREVIEW_HOUR, capture_hour
            ))
    sun_actor = island_clock.get_editor_property("sun")
    sun_component = sun_actor.get_component_by_class(unreal.DirectionalLightComponent) if sun_actor else None
    capture_sun_intensity = float(sun_component.get_editor_property("intensity")) if sun_component else None
    capture_sun_rotation = None
    if sun_actor:
        sun_rotation = sun_actor.get_actor_rotation()
        capture_sun_rotation = [
            round(float(getattr(sun_rotation, axis)), 2) for axis in ("pitch", "yaw", "roll")
        ]
    log("transient Island clock set to {:.2f}; map will not be saved".format(capture_hour))
    log("preview sun intensity={} rotation_pitch_yaw_roll={}".format(
        capture_sun_intensity, capture_sun_rotation
    ))
    mesh = require(unreal.load_asset(ASSET), "Could not load Skeletal Mesh: " + ASSET)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise TypeError("Expected a SkeletalMesh, got " + mesh.get_class().get_name())

    skeletal_component_class = getattr(unreal, "InstancedSkinnedMeshComponent", None)
    if skeletal_component_class is None:
        raise RuntimeError("InstancedSkinnedMeshComponent is not exposed to the UE Python API")
    actor_class = unreal.load_class(None, "/Script/CaptiveSky_2.IslandInstancedSkinnedFoliage")
    require(actor_class, "Native IslandInstancedSkinnedFoliage class is unavailable; build the Editor target first")

    target_actor = next(
        (actor for actor in actor_tools.get_all_level_actors()
         if actor.get_actor_label() == TARGET_LABEL),
        None,
    )
    require(target_actor, "Could not find target landmark: " + TARGET_LABEL)
    anchor = target_actor.get_actor_location()
    trace_x, trace_y = anchor.x + 220.0, anchor.y + 100.0
    trace_start = unreal.Vector(trace_x, trace_y, anchor.z + 2500.0)
    trace_end = unreal.Vector(trace_x, trace_y, anchor.z - 6000.0)
    hit = unreal.SystemLibrary.line_trace_single(
        world,
        trace_start,
        trace_end,
        unreal.TraceTypeQuery.ECC_VISIBILITY,
        False,
        [target_actor],
        unreal.DrawDebugTrace.NONE,
        True,
    )
    require(hit, "Ground trace near {} found no surface".format(TARGET_LABEL))
    ground = hit.to_tuple()[5]
    location = unreal.Vector(ground.x, ground.y, ground.z + 1.0)

    preview_actor = require(
        actor_tools.spawn_actor_from_class(actor_class, location, unreal.Rotator(0, 0, 0), True),
        "Could not spawn transient native foliage actor",
    )
    preview_actor.set_editor_property("source_mesh", mesh)
    identity = unreal.Transform(
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
        unreal.Vector(1.0, 1.0, 1.0),
    )
    preview_actor.set_editor_property("instance_transforms", [identity])
    preview_actor.rebuild_instances()
    component = require(
        preview_actor.get_component_by_class(skeletal_component_class),
        "Native preview actor has no InstancedSkinnedMeshComponent",
    )
    instance_count = int(component.get_instance_count())
    require(instance_count == 1, "Instanced skinned component did not retain exactly one instance")

    standard_actor = require(
        actor_tools.spawn_actor_from_class(unreal.SkeletalMeshActor, location, unreal.Rotator(0, 0, 0), True),
        "Could not spawn a transient ordinary SkeletalMeshActor for comparison",
    )
    standard_component = require(
        standard_actor.get_component_by_class(unreal.SkeletalMeshComponent),
        "Ordinary SkeletalMeshActor has no SkeletalMeshComponent",
    )
    standard_component.set_editor_property("skeletal_mesh_asset", mesh)
    standard_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    standard_component.set_visibility(True, True)
    standard_actor.set_actor_hidden_in_game(False)

    materials = []
    material_count = int(component.get_num_materials())
    for index in range(material_count):
        material = component.get_material(index)
        material_path = material.get_path_name() if material else None
        details = material_parameters(material) if material else None
        materials.append(details)
        log("material[{}]={}".format(index, material_path or "<none>"))
        if details:
            log("material_details[{}]={}".format(index, json.dumps(details, sort_keys=True)))

    base_foliage_material = component.get_material(1)
    preview_color = unreal.LinearColor(*FOLIAGE_TINT)
    foliage_tint_parameters = [
        entry["name"]
        for entry in (materials[1] or {}).get("vector_parameter_values", [])
        if entry.get("name", "").replace(" ", "").lower().startswith("basecolortint")
    ] if len(materials) > 1 and materials[1] else []
    require(foliage_tint_parameters,
            "Foliage material exposes no BaseColor Tint vector parameters")
    for label, foliage_component in (("instanced", component), ("ordinary", standard_component)):
        preview_material = foliage_component.create_dynamic_material_instance(1)
        require(preview_material, "Could not create transient {} foliage MID".format(label))
        for parameter_name in foliage_tint_parameters:
            preview_material.set_vector_parameter_value(parameter_name, preview_color)
        log("{} foliage tint {} applied to transient MID parameter(s) {} from {}".format(
            label, list(FOLIAGE_TINT), foliage_tint_parameters,
            base_foliage_material.get_path_name() if base_foliage_material else "<none>",
        ))

    mesh_bounds = mesh.get_bounds()
    bounds_origin = mesh_bounds.origin
    bounds_extent = mesh_bounds.box_extent
    center = location + bounds_origin
    height = float(bounds_extent.z) * 2.0
    camera_distance = max(600.0, height / 0.43)
    camera_location = center + unreal.Vector(camera_distance * 0.55,
                                              -camera_distance,
                                              camera_distance * 0.30)
    camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, center)
    camera_actor = require(
        actor_tools.spawn_actor_from_class(
            unreal.SceneCapture2D, camera_location, camera_rotation, True
        ),
        "Could not spawn transient scene-capture actor",
    )
    capture = require(camera_actor.get_component_by_class(unreal.SceneCaptureComponent2D),
                       "Scene-capture actor has no capture component")
    capture.set_editor_property("capture_every_frame", False)
    capture.set_editor_property("capture_on_movement", False)
    capture.set_editor_property("fov_angle", FOV_ANGLE)
    capture.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    capture.set_editor_property(
        "primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST
    )
    capture.show_only_component(component)
    render_target = require(
        unreal.RenderingLibrary.create_render_target2d(
            world, 1440, 960, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB,
            unreal.LinearColor(0.12, 0.16, 0.12, 1.0), False,
        ),
        "Could not create render target",
    )
    capture.set_editor_property("texture_target", render_target)

    standard_camera = require(
        actor_tools.spawn_actor_from_class(
            unreal.SceneCapture2D, camera_location, camera_rotation, True
        ),
        "Could not spawn ordinary skeletal comparison scene-capture actor",
    )
    standard_capture = require(
        standard_camera.get_component_by_class(unreal.SceneCaptureComponent2D),
        "Ordinary skeletal comparison capture has no scene-capture component",
    )
    standard_capture.set_editor_property("capture_every_frame", False)
    standard_capture.set_editor_property("capture_on_movement", False)
    standard_capture.set_editor_property("fov_angle", FOV_ANGLE)
    standard_capture.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    standard_capture.set_editor_property(
        "primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST
    )
    standard_capture.show_only_component(standard_component)
    standard_render_target = require(
        unreal.RenderingLibrary.create_render_target2d(
            world, 1440, 960, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB,
            unreal.LinearColor(0.12, 0.16, 0.12, 1.0), False,
        ),
        "Could not create ordinary skeletal comparison render target",
    )
    standard_capture.set_editor_property("texture_target", standard_render_target)

    output_directory = os.path.join(unreal.Paths.project_dir(), OUTPUT_RELATIVE)
    os.makedirs(output_directory, exist_ok=True)
    try:
        transform_provider = component.get_transform_provider()
        transform_provider_path = transform_provider.get_path_name() if transform_provider else None
    except Exception as error:
        transform_provider_path = "Python API unavailable: " + str(error)
    report = {
        "asset": ASSET,
        "mesh_class": mesh.get_class().get_name(),
        "component_class": component.get_class().get_name(),
        "instance_count": instance_count,
        "material_slot_count": material_count,
        "transform_provider": transform_provider_path,
        "materials": materials,
        "transient_foliage_tint": list(FOLIAGE_TINT),
        "applied_foliage_tint_parameters": foliage_tint_parameters,
        "bounds_origin_cm": components(bounds_origin),
        "bounds_extent_cm": components(bounds_extent),
        "height_cm": round(height, 2),
        "target": TARGET_LABEL,
        "capture_hour": capture_hour,
        "capture_sun_intensity": capture_sun_intensity,
        "capture_sun_rotation_pitch_yaw_roll": capture_sun_rotation,
        "map_saved": False,
        "outputs": [
            os.path.join(output_directory, OUTPUT_FILENAME),
            os.path.join(output_directory, STANDARD_OUTPUT_FILENAME),
        ],
        "limitations": [
            "One reference-pose instance only; no transform provider or wind animation was authored.",
            "A scene-capture image is not a runtime draw-cost or LOD measurement.",
        ],
    }
    PREVIEW_STATE.update({
        "ticks": 0,
        "handle": None,
        "captures": [capture, standard_capture],
        "outputs": [
            (OUTPUT_FILENAME, render_target),
            (STANDARD_OUTPUT_FILENAME, standard_render_target),
        ],
        "output_directory": output_directory,
        "report": report,
        "world": world,
    })
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    PREVIEW_STATE["handle"] = unreal.register_slate_post_tick_callback(on_preview_tick)
    log("waiting for renderer to settle before the transient capture")


try:
    main()
except Exception:
    unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
    raise
