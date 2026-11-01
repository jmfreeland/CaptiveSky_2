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

## Runtime water-surface Pawn guard

Follow-up source review found that play start only disabled the named legacy
`OceanPlane`; it did not sanitize a blocking Pawn response on generated
`AWaterBodyOcean` collision hulls or the visible `AWaterZone` mesh. The ocean
subsystem now changes only those surfaces' `ECC_Pawn` response from Block to
Overlap, preserving the water-body overlap/query path used for underwater
effects and the existing wave-height swimming logic. The transient
`IslandOcean` regression forces a stale blocking response on the zone mesh and
on any enabled generated ocean colliders before `OnWorldBeginPlay`, then
asserts startup removes Pawn blocking.

The isolated UE 5.8.3 Editor target compiled and linked successfully after
this guard (5 build actions). Neither this automation nor the saved-map audit
has executed against the new worktree module. The live D: editor remains on its
separate module, and the NullRHI startup crash and sub-4-GiB GPU reserve still
preclude a safe behavior/render claim. Verify the focused test in a validation
editor, then use a bounded real-RHI shoreline traversal when memory headroom
allows.

The transient water fixture now also sweeps a player-sized capsule through the
ocean with the `Pawn` collision channel after the startup guard runs. This is a
more direct check that no water surface returns a blocking hit, complementing
the component-response assertions and saved-map audit. It compiles in the
isolated target; it has not executed under the current NullRHI/editor limits.

## User-authorized restart recheck — 2026-11-01 05:21 UTC

After the user confirmed the Island was saved, the live editor was closed
cleanly and the exact D: `CaptiveSky_2.uproject` was reopened. Startup paused on
the Unreal Zen Storage Server delay prompt; choosing “keep waiting” allowed the
project to load, and the Island map is responsive again. The editor had the
Home Panel overlay open; dismissing it exposed the offshore-facing viewport.

The viewport displays Unreal's red video-memory warning: **634.522 MiB over
budget**. No PIE/Game session or character run was started. The camera still
faces offshore, so this restart did not validate tree silhouettes or ground
scatter. Earlier `ListTextures` evidence recorded about 1.6 GiB of transient
render-target allocations while the idle view's world textures were only about
114 MiB; this is a promising lead, not proof those targets caused today's
warning. Keep the existing 4 GiB headroom gate. Next inspect render-target
lifetime/resolution in a bounded diagnostic and reduce those buffers only if
the owning system and visual trade-off are verified. No map, texture-pool, or
landscape settings were changed during this restart.

## Water automation selection and runtime fixture verification — 2026-11-01

The first post-rebuild attempt to run `CaptiveSky2.Agent.IslandOcean` selected
only `IslandOcean.SavedMapCollision`: once a child test shared the `IslandOcean`
prefix, the runner treated the parent path as a group and omitted the transient
player/swim regression. Renamed that test to
`CaptiveSky2.Agent.IslandOcean.RuntimeSwim`, making both checks independently
selectable. The first actual `RuntimeSwim` run then exposed two fixture issues:
the subsystem begin-play hook was called twice (raising an engine ensure), and
the capsule sweep hit the earlier test character's own collision cylinder.
The fixture now calls the hook once after setting up all water surfaces and
ignores that already-spawned character during the water-only sweep. Failed
sweeps also report the blocking component and impact point.

Rebuilt the isolated `Saved\CS58` UE 5.8.3 editor target successfully and ran
`Automation RunTests CaptiveSky2.Agent.IslandOcean.` under NullRHI with resident
thinking and model requests disabled. The runner found both
`CaptiveSky2.Agent.IslandOcean.RuntimeSwim` and
`CaptiveSky2.Agent.IslandOcean.SavedMapCollision`; both completed with
`Success`, exit code 0. The runtime test exercised generated ocean-collider and
water-mesh Pawn-response cleanup, a player-sized capsule sweep through open
water, surface query/swim entry, swim movement and wake, shallow-shore exit,
and shallow-water walking. The saved-map test loaded `/Game/Maps/Island` and
checked its authored ocean plane, native ocean, and water zone.

This closes the automation-selection gap and verifies the collision path in a
transient NullRHI world. It still does **not** prove that a real player can no
longer walk on water in the rendered Island: the last observed editor warning
showed only 634 MiB of GPU memory over budget, below the established 4 GiB
headroom gate. Do not start PIE for this check until that gate is met; then do
a short real-RHI shoreline entry and open-water traversal and record the result.

**Current-source re-run (2026-11-01 09:59 UTC):** after commit `259e9b91`,
the same two named tests were rerun from the short-path isolated scratch project
with `-NullRHI -NoZen -NoSound -DisablePython -unattended`. Both
`RuntimeSwim` and `SavedMapCollision` again completed with `Success` and process
exit code 0; the log confirms a player-sized capsule crossed the transient open
ocean without a blocking hit and the saved Island collision audit passed. The
whole-device GPU sample was 3,101 MiB free, still below the 4,096 MiB render
reserve, so no PIE or real-RHI traversal was started. Scratch log:
`Saved/CC/Project/Saved/Logs/CaptiveSky_2.log`. This is current-source NullRHI
behavior evidence, not a rendered-play claim.

## Live D: source parity check — 2026-11-01

Read-only comparison of the open D: project's source against this worktree
found two relevant fixes that are present on the pushed branch but absent from
the live project source. D:'s `CaptiveSky_2Character.cpp` enters surface-swim
mode but does not rotate the character mesh horizontally; commit `bf46bdfc`
adds the 90-degree surface-swim pose and restores the authored pose on exit.
That makes an upright swimmer look like a character walking on the water, so
the user's visual report does not by itself prove a collision failure.

D:'s `IslandOceanSubsystem.cpp` disables only the hidden legacy `OceanPlane`;
it lacks the later `AWaterBodyOcean` collision-hull and `AWaterZone` mesh Pawn
response guard from commit `799d1309`. The current worktree's automation
explicitly forces those surfaces to block, calls startup cleanup, and checks a
player-sized capsule sweep; its earlier NullRHI run passed. The open editor is
currently on the Island outside PIE, so this comparison did not observe live
movement mode, capsule hits, or generated-water collision. It establishes a
source-version gap, not which symptom is occurring in the current runtime.

Next, load/rebuild the D: project with both pushed fixes, then perform a short
shore-to-open-water traversal recording movement mode, mesh pose, and the
blocking hit actor. Keep the rendered run behind the existing 4 GiB GPU reserve.
No D: source, content, or map was changed during this audit.
