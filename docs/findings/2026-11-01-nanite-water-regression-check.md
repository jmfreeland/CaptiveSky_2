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
