"""Read-only loaded-texture inventory for the Island map.

Run with UE 5.8 UnrealEditor-Cmd.exe while no other editor for this project is
open:
  UnrealEditor-Cmd.exe CaptiveSky_2.uproject -ExecutePythonScript="Scripts/Inspect-IslandTextureStreaming.py" -unattended -RenderOffscreen -NoZen

This loads Island and asks the engine to print its current streaming pool size
and loaded texture inventory. It never saves the map or assets. The inventory
is not a camera-specific wanted-mip profile; compare it with `stat streaming`
in the actual view before changing texture settings.
"""

import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError("Could not load {} for the read-only texture inventory.".format(MAP_PATH))

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        raise RuntimeError("Island loaded without an editor world; no inventory was requested.")

    unreal.log("[IslandTextureInventory] BEGIN map={} (read-only; no camera-specific wanted-mip claim)".format(MAP_PATH))
    unreal.SystemLibrary.execute_console_command(world, "r.Streaming.PoolSize")
    unreal.SystemLibrary.execute_console_command(world, "ListTextures")
    unreal.log("[IslandTextureInventory] COMPLETE; map and assets were not saved.")


try:
    main()
except Exception:
    unreal.log_error("[IslandTextureInventory] FAILED " + traceback.format_exc())
    raise
