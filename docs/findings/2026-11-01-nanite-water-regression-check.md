# Nanite restart and Island water regression check — 2026-11-01

## Result

After the user confirmed the editor was saved and approved a restart, the main
`D:/Projects - Athena/Unreal/CaptiveSky_2/CaptiveSky_2.uproject` project was
closed cleanly and reopened from its verified Epic Launcher project tile. The
Island loaded in UE 5.8.3 and the editor returned to an `All Saved` state. The
project configuration enables both `r.Nanite.ProjectEnabled=True` and
`r.Nanite.Foliage=True`; the restart applies that setting, but does not establish
that it improves the rendered image or frame time.

## Water/player wiring regression

`IslandOceanTests.cpp` now loads the project's configured
`/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode` class and asserts its
default pawn is a child of `ACaptiveSky_2Character`. This protects the actual
project player-class wiring in addition to the test's transient movement
fixture.

The UE 5.8.3 `CaptiveSky_2Editor` target built successfully in the isolated
`Saved/CS58` scratch project (67 UBT actions). The scratch project's `Source/`
points to this worktree and its `Content/` points to the main D: project, so the
open editor's loaded DLL was not replaced. `CaptiveSky2.Agent.IslandOcean`
passed under NullRHI with agent thinking disabled and zero model requests:
[`Codex_IslandOceanPawnGuard_20261101.log`](../../Saved/CS58/Saved/Logs/Codex_IslandOceanPawnGuard_20261101.log).
The automation exercises the project GameMode default pawn, legacy ocean-plane
collision shutdown, native Water surface querying, and transient swim/fall/
shallow-shore transitions. It is not a player-controlled traversal of the
saved Island and does not settle the reported walk-on-water behavior.

After the approved restart, the live D: editor's Session Frontend also ran
`CaptiveSky2.Agent.IslandOcean` (Automation Test Run 6). Its loaded editor
module reported `Test Completed. Result={Success}` at 03:10:05 in
`Saved/Logs/CaptiveSky_2.log`; the log records disabling legacy `OceanPlane`
collision, entering and leaving surface swimming, and the shallow-shore exit
fixture. This validates the test behavior against the restarted project's
loaded module, but still is not real player traversal and does not replace a
fresh build from this worktree.

After the follow-up restart, the focused test was run once more from Session
Frontend. The Automation Testing Log reports Test Run 5 and
`CaptiveSky2.Agent.IslandOcean` completed with result `Success`. The Output Log
shows the transient shallow-shore and open-water player fixtures executing,
followed by the automation controller's successful result. This confirms the
loaded editor module passes its water/player regression in a transient test
world, including the legacy-plane collision guard and swimming checks. It is
not a rendered shoreline traversal and does not prove that the entire saved
Island has no walkable collision above water; keep a bounded visual shoreline
check open for a later session with adequate GPU headroom.

## Rendered validation gate

After reopening the editor, `nvidia-smi` reported 9,178 MiB used and 2,820 MiB
free out of 12,282 MiB on the RTX 4080 Laptop GPU (44% utilization, 73 °C).
That is below the project's 4 GiB rendered-play gate, so no PIE/Game session
or character feedback run was started. The reported graphics-memory
exhaustion, water traversal, and local vegetation readability therefore
remain unverified in a real-RHI player session.

## Next step

Wait for at least 4 GiB free GPU memory before a bounded real-RHI Island pass.
Use the same player-height route across the shoreline and from Tideglass into
the grove, capturing frame time, whole-device memory, tree silhouettes, and
ground-cover distribution. If headroom stays below that gate, continue with
CPU/source-side foliage-memory and texture-budget reductions rather than
starting another rendered run.

## Follow-up restart (2026-11-01, 03:20 UTC)

At the user's request, the editor was restarted again. A project-browser
selection first surfaced an `Aura` missing-plugin prompt, so that launch was
declined rather than disabling the plugin. Launching the exact main D: `.uproject`
path then opened `CaptiveSky_2` successfully in UE 5.8.3 with the Island loaded.
The new startup log records `LogConfig: Set CVar [[r.Nanite.Foliage:1]]`,
confirming the effective foliage setting after this restart. No rendered play
was started. The latest whole-device sample is 9,623 / 12,282 MiB used
(2,375 MiB free, 49% GPU, 70 °C), still below the 4 GiB gate. Session Frontend's
filter found the IslandOcean test, but this attempt did not produce a completed
test result; treat the previous 03:10 pass as the latest verified automation
result.

## User-approved restart recheck (2026-11-01, 03:43 UTC)

After the user reconfirmed the editor was saved, the live D: editor closed
cleanly and `CaptiveSky_2.uproject` was reopened directly. The editor returned
to `/Game/Maps/Island`; its fresh startup log again records
`bEnableNaniteFoliage:1` and `r.Nanite.Foliage:1`, and MapCheck completed with
0 errors and 0 warnings. The viewport is still aimed offshore, so this does
not visually validate the runtime tree communities. The landscape edit-layer
merge warning recurred (4033x4033 required versus a 1024x1024 batch maximum).

The post-restart `nvidia-smi` sample was 9,599 / 12,282 MiB used, leaving
2,399 MiB free (45% GPU utilization, 72 C). This remains below the 4 GiB
rendered-session gate; PIE/Game and character feedback were not started. The
existing 03:10 `IslandOcean` automation pass remains the latest verified water
test. No map or gameplay settings were edited during this restart.

## User-approved restart recheck — 03:55 UTC

After the user again confirmed the project was saved, the editor closed
cleanly and the exact D: `.uproject` was opened. The new startup log records
`bEnableNaniteFoliage:1` and `r.Nanite.Foliage:1`; `MapCheck` completed with
0 errors and 0 warnings. The Island editor is open again, but its sea-facing
viewport does not show the generated woodland. The 4033x4033 landscape
edit-layer merge warning still exceeds the configured 1024x1024 batch maximum.

A fresh whole-device sample reports 9,599 / 12,282 MiB used (2,399 MiB free),
42% utilization, and 71 C. This is still below the 4 GiB reserve, so no
rendered play or resident feedback session was started. The restart verifies
the project-level Nanite Foliage setting only; it does not validate the
worktree's un-synchronized A/B tree variants, runtime water traversal, frame
rate, or peak memory. Keep the matched daylight woodland/shoreline profile as
the next rendered gate, and do not raise tree counts or cull distances before
that comparison.

## Live play-safety cap audit (2026-11-01)

The reopened D: project's `Config/DefaultGame.ini` and
`AgentPlaySessionSubsystem` source still set the default session to 1,800 real
seconds and 120 model requests; its request clamp is also 120. The tracked
worktree now defaults and hard-caps sessions at 64 requests. The D: project's
`Start-Spectator.ps1` does expose explicit per-run lower limits (0–120), so a
future isolated feedback session can use a 900-second deadline and a 64-request
cap without changing the live project configuration. This only proves the
launcher argument is available; no such session was started, and the current
2.4 GiB free GPU sample remains below the rendered-play gate. Before any
model-active live run, verify its startup log reports the explicit 900/64 caps
and keep its data root isolated. The D: project has not been synchronized with
the worktree's 64-request hard ceiling.

## Saved-map water collision regression

The existing `CaptiveSky2.Agent.IslandOcean` test covers a transient swim
fixture and verifies that play start disables the legacy `OceanPlane`, but it
did not inspect the authored Island's own water geometry. Added
`CaptiveSky2.Agent.IslandOcean.SavedMapCollision`, which loads
`/Game/Maps/Island.Island` and checks that the saved ocean plane, native ocean
collision components, and generated water-zone mesh cannot block the player
channel. This makes the next water audit target the actual saved map instead
of only a synthetic world.

The isolated UE 5.8.3 Editor target compiled and linked successfully (4 build
actions, including `IslandOceanTests.cpp`). The test itself has not run: the
open D: editor loads its separate project module, and the known NullRHI
pre-initialization crash remains. This is compile evidence, not proof that the
saved Island passes the collision assertions or that player traversal now
swims. Run the focused test from an editor that loads this worktree module;
then perform the same real-RHI shoreline traversal only after the 4 GiB GPU
headroom gate is met.
