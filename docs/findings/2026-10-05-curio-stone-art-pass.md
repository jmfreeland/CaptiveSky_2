# Curio stone art pass (2026-10-05)

Persistent pale trail stones and the resident-built cairn are now rendered with the same irregular
`SM_Rock` mesh and `M_Rock` material already used by the Wind Arch presentation. The saved curio
records, stable positions, instance counts, and one-stone-per-day contributions remain authoritative;
this changes only the transient renderer. `M_Rock` is loaded at runtime, so Claude's separate wetness
material work can still drive the shared asset without this change editing `Content/`.

If Starter Content is unavailable, curio stones keep their tinted Basic Shape material and sphere
mesh. The open Seedpod and its husks are unchanged. The test asserts the authored mesh/material when
available, the primitive fallback otherwise, and the cairn's collision/navigation invariants.

Validation update (2026-10-05): after cleaning only the isolated scratch project's stale generated
UHT output, a normal UE 5.8.3 editor-target build completed successfully in 138 actions and linked
`UnrealEditor-CaptiveSky_2.dll`. The earlier reflection-macro diagnostics mapped to stale generated
line numbers; they were not a source error in Claude's in-flight `IslandArrangement.h` changes. The
successful build emitted one unrelated `IslandWeather.cpp` C4996 deprecation warning.

The focused `CaptiveSky2.Agent.IslandCurio` automation test then ran in the isolated scratch editor
and passed. Logs: [`Codex_CurioCleanBuild_20261005.log`](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_CurioCleanBuild_20261005.log)
and [`Codex_CurioArtPass_Automation_20261005.log`](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_CurioArtPass_Automation_20261005.log).

Rendered Game/PIE inspection is still outstanding, so the more natural reading of the small cairn
at resident viewing distance is not yet visually confirmed. The automation verifies mesh/material
selection and collision/navigation invariants, not the appearance of the authored asset in the map.
An isolated offscreen capture was attempted, but the editor process could not write to the host's
user DDC/Zen and shader-worker temporary directories and exited before loading the map. This is an
environment permission limitation; no shared project asset or saved world state was changed.
