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

## Remaining validation

Both direct debug placement and automatic storm-to-wrack placement now work on
the map. Nor has an autonomous resident been shown noticing a piece, navigating
to it, and using `interact`; the successful turnover above was by
`debug_visitor`. That remains the next integration check.

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

Next, verify the real storm-mark trigger in another isolated bounded session,
then test a resident's `move_to` and `interact` path without enabling model
thinking until the code-driven behavior is confirmed.
