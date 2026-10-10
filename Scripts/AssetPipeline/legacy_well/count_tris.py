import bpy
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath="D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/CaptiveSky/ComfyBlender/Well/SM_Well_01.fbx")
m = next(o for o in bpy.data.objects if o.type == "MESH" and not o.name.startswith("UCX"))
print("TRIS", sum(max(1, len(p.vertices) - 2) for p in m.data.polygons), "POLYS", len(m.data.polygons))
