"""Capture one imported static mesh beside a chosen Island landmark, without saving the level.

Run with UE 5.8 UnrealEditor-Cmd.exe, the project uproject, -RenderOffscreen,
and -ExecutePythonScript=Scripts/Preview-GeneratedStaticMesh.py. Optional
environment variables: CAPTIVESKY_PREVIEW_ASSET, CAPTIVESKY_PREVIEW_TARGET_LABEL,
CAPTIVESKY_PREVIEW_OUTPUT (a project-relative path),
CAPTIVESKY_PREVIEW_CAPTURE_SOURCE (an Unreal SceneCaptureSource enum name), and
CAPTIVESKY_PREVIEW_FILENAME. Set CAPTIVESKY_PREVIEW_SCENE_MODE to "full" to
render the Island around the transient preview actor instead of an isolated actor.
"""
import os

import unreal


PREFIX = "[StaticMeshPreview] "
MAP = "/Game/Maps/Island"
ASSET = os.environ.get(
    "CAPTIVESKY_PREVIEW_ASSET",
    "/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/SM_WoodlandBluebells_01.SM_WoodlandBluebells_01",
)
TARGET_LABEL = os.environ.get("CAPTIVESKY_PREVIEW_TARGET_LABEL", "Roost_West")
OUTPUT_RELATIVE = os.environ.get(
    "CAPTIVESKY_PREVIEW_OUTPUT", "Saved/Playtests/Codex_WoodlandBluebellRealRHI_20261012"
)
CAPTURE_SOURCE = os.environ.get("CAPTIVESKY_PREVIEW_CAPTURE_SOURCE", "SCS_FINAL_COLOR_LDR")
OUTPUT_FILENAME = os.environ.get("CAPTIVESKY_PREVIEW_FILENAME", "StaticMesh_RealRHI.png")
CAPTURE_SCENE_MODE = os.environ.get("CAPTIVESKY_PREVIEW_SCENE_MODE", "isolated").lower()


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


require(unreal.EditorLoadingAndSavingUtils.load_map(MAP), "Could not load the Island review map")
world = require(
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
    "Editor world is unavailable",
)
actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
require(actor_tools, "Editor Actor Subsystem is unavailable")
mesh = require(unreal.load_asset(ASSET), "Could not load preview Static Mesh: " + ASSET)
section_count = mesh.get_num_sections(0)
log("lod0_section_count={}".format(section_count))

for index, slot in enumerate(mesh.get_editor_property("static_materials")):
    slot_name = slot.get_editor_property("material_slot_name")
    material = slot.get_editor_property("material_interface")
    material_path = material.get_path_name() if material else "<none>"
    log("asset_slot[{}] name={} material={}".format(index, slot_name, material_path))
    if material:
        try:
            input_node = unreal.MaterialEditingLibrary.get_material_property_input_node(
                material, unreal.MaterialProperty.MP_BASE_COLOR
            )
            if input_node:
                class_name = input_node.get_class().get_name()
                output_name = unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(
                    material, unreal.MaterialProperty.MP_BASE_COLOR
                )
                detail = ""
                if class_name == "MaterialExpressionConstant3Vector":
                    detail = " value={}".format(input_node.get_editor_property("constant"))
                log("base_color_input path={} class={} output={}{}".format(
                    material_path, class_name, output_name, detail
                ))
            else:
                log("base_color_input path={} <none>".format(material_path))
        except Exception as exc:
            log("material_graph_inspection_unavailable path={} reason={}".format(material_path, exc))

target_actor = next(
    (actor for actor in actor_tools.get_all_level_actors()
     if actor.get_actor_label() == TARGET_LABEL),
    None,
)
require(target_actor, "Could not find target landmark: " + TARGET_LABEL)
anchor = target_actor.get_actor_location()
trace_x = anchor.x + 220.0
trace_y = anchor.y + 100.0
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
# HitResult exposes the struct fields through its tuple in UE 5.8's Python
# wrapper; the individual reflected properties are not readable from Python.
ground = hit.to_tuple()[5]
mesh_location = unreal.Vector(ground.x, ground.y, ground.z + 1.0)

# Every actor is transient and the script never invokes any save API. Closing the
# commandlet discards this review placement and leaves the Island asset untouched.
mesh_actor = require(
    actor_tools.spawn_actor_from_object(mesh, mesh_location,
                                        unreal.Rotator(0.0, 0.0, 0.0), True),
    "Could not spawn transient mesh actor from the imported asset",
)
mesh_component = mesh_actor.get_component_by_class(unreal.StaticMeshComponent)
require(mesh_component, "Preview actor has no StaticMeshComponent")
mesh_component.set_static_mesh(mesh)
mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
mesh_component.set_visibility(True, True)
mesh_component.set_hidden_in_game(False)
mesh_actor.set_actor_hidden_in_game(False)
for index in range(mesh_component.get_num_materials()):
    material = mesh_component.get_material(index)
    log("component_slot[{}] material={}".format(
        index, material.get_path_name() if material else "<none>"
    ))

look_at = mesh_location + unreal.Vector(0.0, 0.0, 17.0)
camera_location = look_at + unreal.Vector(72.0, -92.0, 58.0)
camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, look_at)
camera_actor = require(
    actor_tools.spawn_actor_from_class(unreal.SceneCapture2D, camera_location,
                                       camera_rotation, True),
    "Could not spawn transient scene-capture actor",
)
capture = camera_actor.get_component_by_class(unreal.SceneCaptureComponent2D)
require(capture, "Scene-capture actor has no capture component")
capture.set_editor_property("capture_every_frame", False)
capture.set_editor_property("capture_on_movement", False)
capture.set_editor_property("fov_angle", 48.0)
capture.set_editor_property("capture_source", getattr(unreal.SceneCaptureSource, CAPTURE_SOURCE))
if CAPTURE_SCENE_MODE == "isolated":
    capture.set_editor_property(
        "primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST
    )
    capture.show_only_component(mesh_component)
elif CAPTURE_SCENE_MODE == "full":
    capture.set_editor_property(
        "primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_RENDER_SCENE_PRIMITIVES
    )
else:
    raise RuntimeError("CAPTIVESKY_PREVIEW_SCENE_MODE must be 'isolated' or 'full'")

render_target = unreal.RenderingLibrary.create_render_target2d(
    world,
    1440,
    960,
    unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB,
    unreal.LinearColor(0.12, 0.16, 0.12, 1.0),
    False,
)
require(render_target, "Could not create render target")
capture.set_editor_property("texture_target", render_target)
capture.capture_scene()

output_directory = os.path.join(unreal.Paths.project_dir(), OUTPUT_RELATIVE)
os.makedirs(output_directory, exist_ok=True)
unreal.RenderingLibrary.export_render_target(
    world, render_target, output_directory, OUTPUT_FILENAME
)
log("map={} target={} asset={} source={} scene_mode={} location={} output={}/{}".format(
    MAP, TARGET_LABEL, ASSET, CAPTURE_SOURCE, CAPTURE_SCENE_MODE, mesh_location,
    output_directory, OUTPUT_FILENAME
))
