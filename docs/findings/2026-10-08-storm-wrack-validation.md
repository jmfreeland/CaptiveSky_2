# Storm wrack validation (2026-10-08)

`CaptiveSky2.Agent.IslandWrack` passed in an isolated UE 5.8.3 scratch project
using the same `IslandWrack.cpp`, `IslandWrack.h`, and `IslandWrackTests.cpp`
as the committed source (SHA-256 matched). The focused test covers deterministic
kind/heap selection, descriptions and aging, turning an item and remembering
who found it, JSON round-tripping and rejection of malformed state, reclamation
by the tide, and the shore's 24-item cap. Log:
[`Codex_WrackValidation_MemoryDDC.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/Codex_WrackValidation_MemoryDDC.log).

## Runtime placement and turnover

The saved Island was loaded in an isolated UE 5.8.3 `UnrealEditor-Cmd` Game
session using `-NullRHI`, an isolated data root, disabled agent thinking, and a
60-second real-time cap. `Island.WrackStorm 5` called the production
`DepositAfterStorm` path and logged five placed pieces along the shoreline,
including their world positions. The command quit after the probe; it made no
model requests. Log:
[`Island.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/Island.log).

A second process reopened that same scratch data root and ran
`Island.WrackTurn 1`. It reported the kelp turned over and a fishing-line find;
`Island.wrack.json` persisted `turned: true` and `by: "debug_visitor"`, and
`chronicle.jsonl` received a `wrack_turned` entry. This exercises the actual
subsystem's save/reload/examination behavior, but the visitor was the developer
command, not an autonomous character. Log:
[`Turn.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/Turn.log).
Persisted ledger:
[`Island.wrack.json`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/World/WorldState/Island.wrack.json).

The subsequent rendered UE 5.8.3 standalone session used the same isolated
world and a camera aimed at the saved items. It reached world-ready in 21
seconds, captured ten 1600x900 frames, then ended after 60.3 real seconds with
zero model requests. The close view shows the turned kelp and neighboring
driftwood visibly resting at the waterline:
[`001_Wrack_Kelp_Find.png`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/BeachScreenshots_Focus/001_Wrack_Kelp_Find.png).
Log:
[`BeachFocus.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/BeachFocus.log).

## Automatic storm-trigger probe

The first forced-storm runtime probe is **not evidence about the current
storm-to-wrack hook**: inspection afterward showed that its isolated scratch
project still contained an older `IslandWeatherTraces.cpp` without the
`UIslandWrackSubsystem` call. It did record the forced storm and advance
`last_storm_mark`, but the compiled binary could not have deposited wrack
automatically. Its result is retained for provenance only:
[`AutomaticStorm.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackAutomaticStorm/AutomaticStorm.log).

After copying the current hook into the isolated scratch project's source,
UE 5.8.3 compiled `IslandWeatherTraces.cpp` and `IslandWrack.cpp` and linked
successfully. A fresh bounded Game session with `Island.Storm 180` then logged
`Storm wrack deposit completed: 5 item(s) placed`; five objects were written
to the isolated `Island.wrack.json` and a `wrack` chronicle event was recorded.
This confirms the real automatic storm path works on the saved Island. The
first run was a stale-binary issue, not a production bug. Logs now report the
placement count and explain early exits (missing ocean, missing landscape,
unsuitable beach, or no valid item positions), so any future map-specific
failure is diagnosable.

Evidence:
[`Codex_WrackDiagnostics_Build_20261008.log`](../../Saved/Logs/Codex_WrackDiagnostics_Build_20261008.log),
[`AutomaticStorm.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackAutomaticStorm_CurrentHook/AutomaticStorm.log),
[`Island.wrack.json`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackAutomaticStorm_CurrentHook/World/WorldState/Island.wrack.json),
and
[`chronicle.jsonl`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackAutomaticStorm_CurrentHook/World/WorldState/chronicle.jsonl).

The existing `CaptiveSky2.Agent.WeatherTraces` automation also passed with the
updated hook. Its lightweight synthetic world intentionally has no ocean plane;
the new warning identifies that expected early return while the weather-trace
assertions still pass:
[`Codex_WrackDiagnostics_WeatherTraces_20261008.log`](../../Saved/Logs/Codex_WrackDiagnostics_WeatherTraces_20261008.log).

## Raven-to-wrack interaction

A bounded UE 5.8.3 Game session forced a storm and then issued the raven's normal
`move_to` action for `Wrack_1`, followed by its normal `interact` action. The
raven flew 55,674 cm from its ordinary spawn, reached the shore in 185.5
simulated seconds, and turned over a kelp heap, finding fishing line. The
isolated ledger records `turned: true` and `by: "Agent_Raven_01"`; the
chronicle records the `wrack_turned` event. The run ended normally after 211.5
real seconds with zero model requests. Its hard real-time cap was 240 seconds.
Evidence:
[`RavenInteract.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRavenInteractionProbe_CurrentHandler_20261008/RavenInteract.log),
[`Island.wrack.json`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRavenInteractionProbe_CurrentHandler_20261008/World/WorldState/Island.wrack.json),
and
[`chronicle.jsonl`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRavenInteractionProbe_CurrentHandler_20261008/World/WorldState/chronicle.jsonl).

This proves that a resident can use the real flight and interaction chain to
change and remember a storm object. The same test exposed an important
grounded-navigation limit: an Innkeeper `move_to` at the shoreline fails with
"no walkable ground near that marker," and `Island.NavProbe` found no baked
navmesh at the wrack or within a 10 m ring. The raven's flight is therefore the
only validated resident route to this shore activity; Aster and other grounded
residents still need an accessible route. Do not call the static-prototype
result from the first Raven run a product failure: that scratch binary had an
older `IslandInteractionUtility.cpp` without the current wrack handler. The
final run rebuilt that handler and verified the saved ledger and chronicle.

A follow-up saved-navmesh scan sampled six heights every 50 m along the
westward line at Y=1035.2 m. None of the 72 points from X=-1000 m through
X=-450 m projected to navigation. A parallel line at Y=1009.6 m found a
walkable point at X=-1000 m, Z=27.8 m, with an 18 m complete route to the
Listening Stones; the next sample 50 m west had no navigation, as did all
tested points toward the coast. These are corridor samples, not a proof that
the whole island lacks other navmesh patches, but they show the current baked
walker network is confined to a small area around the spawn/Listening Stones
and does not reach the shore. Logs:
[`Navmesh.log` (Y=1035.2 m)](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackNavmeshTransition_20261008/Navmesh.log)
and
[`Navmesh.log` (Y=1009.6 m)](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackNavmeshRouteY100960_20261008/Navmesh.log).

The probe's movement deadline is now 210 simulated seconds so long Raven
flights can finish while remaining bounded. The final run took 211.5 real
seconds overall, including startup. A future integration improvement should
extend safe grounded navigation through the relevant island corridors and at
least one shore approach, then validate Aster's approach and interaction with a
similarly capped run. Avoid changing wrack coordinates to conceal the map's
missing navigation coverage.

Early attempts exposed environment-specific setup issues: the default scratch
launch could not read the shared Zen/Derived Data Cache; a shader compiler
transfer directory under `C:\Users\freel\UnrealShaderWorkingDir` was
inaccessible; and a later scratch DDC path exceeded UE's 119-character limit.
The successful rendered run used UE's normal cache and completed normally.
The open interactive editor and unsaved level were left untouched; project
settings and `Content/` were not changed. Diagnostic logs from the earlier
failed attempts remain available:
[`Codex_WrackValidation.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/Codex_WrackValidation.log),
[`Spectator.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRuntimeWorld/Spectator.log),
[`Spectator_LocalCache.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRuntimeWorld/Spectator_LocalCache.log),
[`BeachCapture_ShortPaths.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/BeachCapture_ShortPaths.log),
[`BeachCapture_Warmed.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackHeadlessRuntime/BeachCapture_Warmed.log).

The automatic storm-mark trigger and Raven-to-wrack action chain are now
verified. The remaining task for this feature is a grounded path to the shore
and an Aster interaction test; keep agent thinking off until those physical
behaviors are confirmed.

## Bounded Raven shore awareness

The Raven's normal situation summary now includes a coarse, optional cue for
fresh unturned storm wrack within 600 m. It names the nearest `Wrack_N` target,
gives an approximate distance, and says to fly before inspecting at close
range. Additional nearby heaps are mentioned without replacing the normal
25 m local observation. The distant cue is only added for a Raven controller;
grounded residents keep their existing local-only perception. Turned, expired,
future-dated, and out-of-range items are omitted. This bridges the gap between
the demonstrated ~557 m spawn-to-shore flight and the Raven's ordinary local
perception without making the chronicle a source of character knowledge.

The helper's focused automation assertions passed in an isolated UE 5.8.3
scratch project, and the scratch editor target compiled and linked successfully.
This verifies cue filtering and the compiled integration call site, not whether
an LLM-driven Raven will choose to follow the invitation in a live session;
agent thinking was intentionally disabled in physical probes to keep them at
zero model requests. The in-world decision remains to be tested after the
grounded shore route and map-navmesh question are resolved.

An autonomous-choice check was attempted with the compiled scratch project,
five seeded wrack items, an isolated output root, NullRHI, a 240-second play
cap, and a five-request ceiling. Both `UnrealEditor.exe` and
`UnrealEditor-Cmd.exe` exited with code 3 before producing a game log or
populating their isolated world roots. No model request was made and the open
interactive editor/map was untouched. Recent Application/WER queries had no
matching crash report. This is a launch failure, not evidence that the Raven
ignored the cue; the autonomous decision is still unverified. Resume only from
a launch path that produces `LogInit` and `LogAgentSession` output, and retain
the short realtime/request caps.

## Ground navigation bounds experiment

An isolated audit of the saved map found one `NavMeshBoundsVolume` centered at
`(-100900, 100460, 3920)` cm with half-extents `(5000, 5000, 2000)` cm. The
storm-wrack point `(-44480.88, 103520.22, 1069.24)` is about 564 m west of
that volume center, outside its 100 m square footprint. The saved
`RecastNavMesh-Default` was `STATIC`, with 10 m tiles and a 35 cm agent radius.
Audit log:
[`IslandNavBoundsAudit.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/IslandNavBoundsAudit.log).

To test the obvious coverage hypothesis without touching the open map, a
physical scratch copy of `Island.umap` was changed to scale the volume by
`(12, 3, 2)` (600 m by 150 m half-extents) and set the Recast data to `DYNAMIC`.
The main `Content/Maps/Island.umap` hash remained unchanged. In a bounded
four-minute `-NullRHI` Game run, the runtime build reported no tile work (0.00
seconds, zero remaining tasks), and the shore point still did not project to
navigation. A follow-up settings audit confirmed the scratch Recast data was
indeed dynamic, with a 1024-tile pool, and that
`GenerateNavigationOnlyAroundNavigationInvokers` was false. So this failed
probe is not explained by the test accidentally remaining static or by
invoker-only generation. It does **not** rule out a proper offline editor bake;
it shows that simply expanding bounds and asking the runtime to rebuild is not
enough in this project. Evidence:
[`NavBoundsRuntimeProbe.log`](../../Saved/NavBoundsTest/Project/Saved/Logs/NavBoundsRuntimeProbe.log),
[`IslandNavRuntimeSettingsAudit2.log`](../../Saved/NavBoundsTest/Project/Saved/Logs/IslandNavRuntimeSettingsAudit2.log).

The next validating step is an offline `Build Paths` bake in an isolated editor
copy, then `Island.NavProbe` at both the wrack and a corridor point. If that
works, the actual map needs an expanded bounds volume **and saved baked
navigation**; do not save or alter the live Island while its editor state is
unsaved. Epic documents static navigation as offline/saved and dynamic
navigation as supporting runtime generation ([generation modes](https://dev.epicgames.com/documentation/unreal-engine/overview-of-how-to-modify-the-navigation-mesh-in-unreal-engine),
[basic navigation and Build Paths](https://dev.epicgames.com/documentation/unreal-engine/basic-navigation-in-unreal-engine?lang=en-US)).
