"""Idempotently place the BlueJar pickup prop on the Island inn's front table."""
import json
import os

import unreal


LEVEL = "/Game/Maps/Island"
MESH_PATHS = (
    "/Game/Generated/ComfyBlender/BlueJar_Refined_20261123/SM_BlueJar_01.SM_BlueJar_01",
    "/Game/Generated/ComfyBlender/BlueJar/SM_BlueJar_01.SM_BlueJar_01",
)
OUTPUT = os.path.join(unreal.Paths.project_saved_dir(), "BlueJarPlacement.json")


def actor_tags(actor):
    return {str(tag) for tag in actor.tags}


if not unreal.EditorLevelLibrary.load_level(LEVEL):
    raise RuntimeError("Could not load " + LEVEL)

actors = unreal.EditorLevelLibrary.get_all_level_actors()
existing = next((actor for actor in actors if "BlueJar" in actor_tags(actor)), None)
table = next((actor for actor in actors if actor.get_actor_label() == "Inn_Table_Front"), None)
if not table:
    raise RuntimeError("Inn_Table_Front was not found in Island")

mesh = next((unreal.load_asset(path) for path in MESH_PATHS if unreal.load_asset(path)), None)
if not mesh:
    raise RuntimeError("Import the BlueJar asset before running this placement script")

table_origin, table_extent = table.get_actor_bounds(False, True)
table_location = table.get_actor_location()
table_top = table_origin.z + table_extent.z
if existing:
    jar = existing
else:
    initial_location = unreal.Vector(table_location.x, table_location.y, table_top)
    jar = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, initial_location, table.get_actor_rotation())
    if not jar:
        raise RuntimeError("Could not spawn the BlueJar actor")

component = jar.get_component_by_class(unreal.StaticMeshComponent)
if not component:
    if not existing:
        unreal.EditorLevelLibrary.destroy_actor(jar)
    raise RuntimeError("BlueJar actor has no static mesh component")
component.set_static_mesh(mesh)
component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
jar.set_actor_label("BlueJar")
jar.set_editor_property("tags", [unreal.Name("BlueJar")])

jar.set_actor_rotation(table.get_actor_rotation(), False)
jar.set_actor_location(unreal.Vector(table_location.x, table_location.y, table_top), False, True)
jar_origin, jar_extent = jar.get_actor_bounds(False, True)
pivot_to_bounds_min_z = jar_origin.z - jar_extent.z - jar.get_actor_location().z
placement = unreal.Vector(table_location.x, table_location.y,
                          table_top - pivot_to_bounds_min_z + 1.0)
jar.set_actor_location(placement, False, True)
if not unreal.EditorLevelLibrary.save_current_level():
    if not existing:
        unreal.EditorLevelLibrary.destroy_actor(jar)
    raise RuntimeError("Island map save failed")
existing = jar
print("Placed BlueJar on Inn_Table_Front using " + mesh.get_path_name())

result = {
    "map": LEVEL,
    "label": existing.get_actor_label(),
    "tags": sorted(actor_tags(existing)),
    "location": [existing.get_actor_location().x, existing.get_actor_location().y,
                 existing.get_actor_location().z],
    "mesh": existing.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_path_name(),
}
with open(OUTPUT, "w", encoding="utf-8") as stream:
    json.dump(result, stream, indent=2)
print("BLUEJAR_PLACEMENT=" + json.dumps(result, sort_keys=True))
