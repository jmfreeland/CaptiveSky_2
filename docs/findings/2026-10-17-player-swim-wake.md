# Player surface-swim wake — 2026-10-17

## Change

The player now leaves a short procedural ring wake while surface swimming, but
only while moving horizontally at 80 cm/s or faster. A successful wake spawn is
rate-limited to one every 1.15 seconds, placed 48 cm behind the swimming
direction at the queried ocean surface. It uses a 1.1-second, 82 cm ring with
a restrained aqua emissive tint and zero point-light intensity. It is tagged
`PlayerSwimImpact` and does not call the Tideglass material-ripple subsystem,
so this open-ocean player cue stays separate from the prototype pool and
its material ripple. The minnow school has a separate ecological response to a
nearby surface-swimming player: it briefly scatters within its 5 m startle
radius, observes an 8-second cooldown, and rearms after the player leaves. The
wake itself makes no LLM or network request.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` scratch build completed successfully
with 173/173 actions, including `CaptiveSky_2Character.cpp`,
`IslandPoolRippleEffect.cpp`, and `IslandOceanTests.cpp`. Follow-up incremental
builds also succeeded after the player-only tint change and after adding a test
assertion that the visible wake ring has a material.

The focused NullRHI automation launch did not reach the test. Two launches of
the scratch project ended at `SecondsSinceStart=0` with a null-address access
violation in `UnrealEditor-Core`; the second launch used scratch-relative DDC,
shader-work, and log paths to rule out the spaced absolute paths as the cause.
Both recorded stack hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`, matching the
previously documented unsymbolized UE commandlet startup crash. The fresh dump
is at `Saved/CompileScratch/Codex_PlayerSwimWake_20261017/Saved/Crashes/_0000/`.
There are no matching editor PDBs in the installed UE 5.8.3 `Win64` binaries,
so this evidence cannot identify the root cause. The wake assertions compiled
but have not run, and the wake has not had a visual in-game review. The adjacent
minnow behavior is covered by the existing
`CaptiveSky2.Agent.TidepoolMinnows` test, but that test has not been rerun in
the current startup environment.

The user's existing Unreal editor was left untouched. Do not repeat this
commandlet launch until the startup stack can be symbolized or a meaningfully
different, safe editor-driven validation route is available. Next: resolve the
matching engine dump, then run `CaptiveSky2.Agent.IslandOcean` and visually
check the wake in open-water play before treating the interaction as verified
or shipping it.

## Live-editor follow-up (2026-10-22)

After reloading the clean `/Game/Maps/Island`, `CaptiveSky2.Agent.IslandWeather`
passed. `CaptiveSky2.Agent.IslandOcean` still failed only the moving-wake count.
The current editor log shows the open-water test character entering surface
swimming and then leaving during the movement tick, before a wake is spawned;
the exact shore-trace hit is not logged. Two assertions were added immediately
after that tick to report whether the swimmer remains in surface mode and keeps
at least the 80 cm/s wake threshold. Live Coding compiled successfully.

The next automation run then terminated the editor with an Unreal class-type
assertion in `UWorld::GetSubsystem<UIslandOceanSubsystem>` at
`IslandOceanTests.cpp:29` (`Ret->IsA(T::StaticClass())`), following Live Coding
re-instancing. The map had been clean before reload. The editor/MCP connection
is currently unavailable, and no fresh post-crash project log was produced by
the restart attempts. Treat the diagnostic assertions and the swim-wake path
as unverified; resume with a clean editor process before changing gameplay
logic based on the stale-class crash.

## UE 5.8 relaunch follow-up (2026-10-22)

The main `CaptiveSky_2Editor` target rebuilt successfully from the current
working tree (14 actions, including `IslandOceanTests.cpp`). The editor then
opened to the UE 5.8 Project Browser, where `CaptiveSky_2` appeared as the
current recent project; it did not automatically reopen `/Game/Maps/Island`.
The Project Browser window became unavailable to computer-use input when the
project tile was selected, and a fresh window inventory no longer listed
Unreal. No new project log or crash report was produced. This corrects the
earlier inference that the relaunch itself had crashed: the project editor was
not yet reached, so the Island reload, ocean test, species scatter, graphics
memory, and walking-on-water checks remain unverified.

## Reloaded-editor check (2026-10-22)

The main project reopened `/Game/Maps/Island`. A short Simulate pass logged
`IslandOcean: disabled collision on legacy OceanPlane` before world startup, so
the hidden plane guard is active in this editor build. This does not prove the
player can enter and remain in surface-swim mode. The focused
`CaptiveSky2.Agent.IslandOcean` run still fails two checks after the open-water
test character moves: it is no longer in surface-swim mode and no wake is
spawned. The transient test log shows the character entering and then leaving
swimming, but does not record which query or trace caused the exit. Do not
interpret the plane-collision guard as resolving the user's water-walking report.

No player-controlled PIE run was attempted after the short Simulate pass reached
11,419 / 12,282 MiB GPU memory. The next diagnosis should instrument the actual
open-water query/shore-exit branch (with the editor on a clean, low-memory run),
then verify on the Island with player input. The current C++ source changes and
map remain separate from that unverified gameplay behavior.

## Transition diagnostics (2026-10-22)

Added transition-only logs in `ACaptiveSky_2Character::Tick` for the three
possible swim exits: missing ocean component, failed ocean-surface query, and
successful shallow-shore exit (including hit actor/component, depth, and
location). This is diagnostic instrumentation only; it does not change the
swimming decision. Unreal MCP could not connect at `127.0.0.1:8000`, and the
computer-use surface returned no native apps. The approved UE 5.8 editor-target
build compiled `CaptiveSky_2Character.cpp`, but linking the main project DLL
failed because the open `UnrealEditor.exe` process holds it. No process was
terminated. The instrumentation passed `git diff --check` and compiled, but the
full target is not linked and the runtime behavior is not verified.

## Rebuilt player-water regression (2026-10-22)

After the user approved a reload/rebuild, the current UE 5.8 editor target
linked successfully and reopened `/Game/Maps/Island`. The focused
`CaptiveSky2.Agent.IslandWeather` test passed (2.74 s); its temporary world has
no Island landscape, so its “no ground-cover instances placed” warning does
not validate scatter.

The freshly rebuilt `CaptiveSky2.Agent.IslandOcean` test first reproduced two
failures: the open-water swimmer left swim mode while moving, and no wake was
spawned. The log identified the cause: the shore-depth line trace hit another
test character's `CollisionCylinder` 4 cm below the surface and treated that
pawn as shallow ground. `ACaptiveSky_2Character::Tick` now ignores pawn actors
for this ground-only query while retaining the walkable-floor and capsule
clearance checks. After a clean rebuild/relaunch, the same focused ocean test
passed (0.039 s, no warnings).

Read-only checks on the reloaded Island confirm both `WaterZone_1.WaterMesh`
and the legacy `OceanPlane` have `NoCollision`; the plane also has
`bUseDefaultCollision=false`. `/Game/Maps/Island` reports clean. The editor
viewport remains a wide editor-only view; no PIE was started because the
project has a reported graphics-memory exhaustion issue and the available
current-session log only reports the configured 4096 MiB texture pool, not an
actual over-budget event. Consequently, the passing test verifies swim-state
retention and wake creation in its transient fixture, but player-controlled
shore-to-open-water traversal, the 30-FPS target, current GPU headroom, and
runtime vegetation appearance remain unverified.

## Capped editor-PIE traversal probe (2026-10-22)

The current editor remains responsive, but a standalone `-game` launch hit the
same pre-engine null access violation recorded in prior failed project starts:
`SecondsSinceStart=0`, `PCallStackHash=4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`.
Do not repeat that standalone launch without a meaningful environment change.

Instead, a short in-editor PIE probe temporarily changed only the
`AgentPlaySessionSubsystem` class defaults in editor memory to a 90-second,
zero-model-request cap, then spawned the normal player at `(500000, 500000,
5940)` in open water. The live log confirmed the cap and reported
`BP_ThirdPersonCharacter_C_0 entered surface swimming at Z=940.0`. PIE was
stopped after 32.8 real seconds with zero requests/in-flight work when `nvidia-smi`
showed 10,932 / 12,282 MiB allocated. No `LogRHI` out-of-memory message or
texture-pool-over-budget event appeared; the streaming log only reported the
configured 4096 MiB pool. After PIE stopped, GPU allocation fell to 5,802 MiB,
`/Game/Maps/Island` remained clean, and the session defaults were restored to
1800 seconds / 120 requests.

This confirms that the actual Island player enters the custom swim mode offshore
in the current editor build, in addition to the passing transient regression.
It does not yet show input-driven horizontal travel, shore return, or a rendered
gameplay capture. The near-limit GPU allocation during PIE is direct evidence
that longer RHI sessions need a memory guard even though the engine emitted no
explicit exhaustion warning. The editor-only viewport still shows an unscattered
surface and is not evidence for runtime plant distribution.
