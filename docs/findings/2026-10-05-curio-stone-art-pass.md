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

An isolated offscreen `CaptiveSky2.Visual.Viewpoints` capture then passed at both noon and 17:00,
using the scratch project's own read-only world-state fixture with agent thinking disabled. The
[noon close-up](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-05_185612_h12.0/Codex_CairnDetail.png)
confirms the irregular mesh and authored material render. At this very close angle the pile reads
larger and more shadow-contrasted than intended; this is a diagnostic, not a final highlight or a
PIE/playtest confirmation. A resident-scale shot in the saved map is still needed before treating
the visual pass as settled. Log: [`Codex_CairnDetail_Noon_20261005.log`](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_CairnDetail_Noon_20261005.log).

The first attempts could not write to the host DDC/Zen and shader-worker temp directories. Keeping
DDC and shader temp under the ignored scratch project (`-DDC-ForceMemoryCache`, process-local
`TEMP`/`TMP`, and `-shaderworkingdir`) resolved that environment permission issue; no shared project
asset or saved world state was changed.
