"""Export the Fab Crow and Fox skeletal meshes to FBX for Scripts/AssetPipeline/creatures/{raven,rat}.py.

  UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Export-CreatureReferences.py -RenderOffscreen
(skeletal-mesh export asserts under -NullRHI). Output: CREATURE_REF or <ComfyBlender>/Creatures/ref/.
"""
import os
import traceback

import unreal

OUT = os.environ.get("CREATURE_REF") or os.path.join(unreal.Paths.project_dir(), "Saved", "CaptiveSky", "ComfyBlender", "Creatures", "ref")
os.makedirs(OUT, exist_ok=True)
try:
    for name, path in (("SK_Crow", "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow"), ("SK_Fox", "/Game/AnimalVarietyPack/Fox/Meshes/SK_Fox")):
        task = unreal.AssetExportTask()
        task.object = unreal.load_asset(path)
        task.filename = os.path.join(OUT, name + ".fbx")
        task.automated = True
        task.replace_identical = True
        task.prompt = False
        unreal.log("CreatureExport {} -> {}".format(name, unreal.Exporter.run_asset_export_task(task)))
except Exception:
    unreal.log_error("CreatureExport FAILED\n" + traceback.format_exc())
