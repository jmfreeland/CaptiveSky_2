# Wet landscape texture-index warning follow-up (2026-10-29)

## Finding

A bounded in-editor PIE run reproduced this warning from the wet landscape
material:

```text
M_Island_Textured_Wet: Requesting an invalid TextureIndex! (21 / 20)
```

The UE 5.8 engine source at
`Engine/Source/Runtime/Engine/Private/Materials/MaterialUniformExpressions.h`
shows this warning is emitted by `GetIndexedTextureCollection` when a material
requests a texture-collection index outside
`Material.GetReferencedTextureCollections()`. The wording is therefore about
an invalid texture-collection index, not by itself a conventional texture
sampler-count or GPU-memory warning. This does not establish a visible defect
or connect the warning to the user's graphics-memory exhaustion reports.

The wet material had 235 inspected expressions. Recursive inspection of its
29 referenced material graph/function graphs found no explicit
`TextureCollection` expression. Asset-registry dependencies likewise did not
expose a texture-collection asset. The origin of index 21 remains unknown; a
compiled/generated binding or stale material state is still only a hypothesis.

## Diagnostic attempt and editor state

`MaterialTools.recompile` was invoked on
`/Game/Materials/M_Island_Textured_Wet.M_Island_Textured_Wet`; the tool returned
without diagnostics. The existing warning remained in the session log, so this
is not evidence of a fix. Afterwards Unreal reported the wet material dirty
while `/Game/Materials/MI_Island_Landscape` remained clean. No material or map
was saved; leave the material unsaved until its dirty state and origin are
understood. PIE is stopped and the editor remains on the Island.

The PIE session ran for about 35.5 seconds and started four resident-model
requests before it was stopped; none remained in flight. The safety limit was
1,800 real seconds / 120 requests, but startup requests were immediate. Avoid
another ordinary PIE launch for material-only diagnosis; if runtime validation
becomes necessary, set `CaptiveSky.Agent.DisableThinking=1` before starting.

The concurrent whole-device GPU sample showed 8,633 / 12,282 MiB in use, 73°C,
and 54% utilization. WDDM exposed no per-process rows, so this cannot be
attributed to Unreal. No controlled FPS, frame-time, or peak-VRAM sample was
collected. Startup also logged repeated foliage-material warnings for missing
`InstancedStaticMeshes` usage permutations, including Rhododendron and generated
marsh-plant materials; those remain a separate asset-usage investigation.

## Next diagnostic

Compare the wet material's serialized/compiled texture-collection references
with its parent `M_Island_Textured_Auto` and sibling shore material, and inspect
the compiler/uniform-expression data that supplies index 21. Reproduce only
with agent thinking disabled and sufficient GPU headroom. Do not save a
material change until the specific stale or invalid binding is identified;
record warnings separately from measured rendering performance.
