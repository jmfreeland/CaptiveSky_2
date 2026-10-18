# Common Frog Tideglass resident — 2026-10-18

## Change

`AIslandCommonFrog` is a small, transient wildlife actor attached to the
existing `TideglassPool` world feature. At Game/PIE start, the Tideglass
subsystem attempts to place up to two frogs just outside the generated pool's
outer shoreline ring, using static-world ground traces; it does not save or
modify the map. Actors are collisionless, do not affect navigation, and make no
LLM requests.

When the exported candidate mesh is already imported at its expected path, the
actor uses it. Otherwise, a tiny color-blocked engine-sphere fallback preserves
the silhouette. This is deliberately a fallback, not the final art review.

## Behavior

- A nearby player, settled Raven, or awake Rat (within 240 cm horizontally and
  180 cm vertically) causes one short hop away per approach. The visitor must
  retreat beyond 520 cm to rearm. A shared seven-second cooldown prevents
  overlapping disturbances from chaining hops; flying Ravens and sleeping Rats
  do not startle it.
- Hop arcs last 0.48 seconds and stay within 420 cm of the animal's original
  habitat. Ground destinations must pass a static-world trace and slope/height
  bounds.
- Ambient hops are checked infrequently; rain or night increases activity,
  while dry daylight remains quiet. At most two instances are created.
- All movement and actor lifetime are session-local; the saved Island and
  persistent resident state remain unchanged.

## Validation and limits

`Source/CaptiveSky_2/Agent/Tests/IslandCommonFrogTests.cpp` covers observer
limits, rearming distance, invalid inputs, activity ordering, and hop shape.
The source-only UE 5.8.3 target
`Saved/CompileScratch/Codex_CurrentSourceBuild_20261018` rebuilt successfully
after adding the Raven/Rat/player approach checks and Rat's read-only awake
query. UHT completed, and the frog actor, Rat controller, and automation
translation units compiled and linked. The automation cases themselves have
not run in this pass.

The targeted automation did not start: `UnrealEditor-Cmd.exe` crashed at
`StartOfEnginePreInit` with a null-address access violation before test
discovery. Its `PCallStackHash` is
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`, matching earlier scratch
commandlet startup crashes already in `Saved/`. The user confirms the interactive
editor is open and responsive, but the desktop bridge does not expose its window;
the editor was left untouched. No local `UnrealEditor-Core.pdb`
or `UnrealEditor-Cmd.pdb` is installed, so the raw Core offsets cannot be
symbolized here. The report and log are in
`Saved/CompileScratch/Codex_CurrentSourceBuild_20261018/Saved/Crashes/_0000/`
(`CrashContext.runtime-xml`, `UEMinidump.dmp`, and `CommonFrogEcology.log`).
This is an engine/project startup failure, not a failed frog assertion. No
Unreal window is exposed by the desktop bridge, so no editor interaction,
import, PIE run, or map save was attempted. Real-RHI appearance, player
interaction, and imported-mesh scale remain pending.

The next gate is to resolve the pre-test startup crash, run
`CaptiveSky2.Agent.CommonFrogEcology`, then review one live session at the
existing Tideglass camera and confirm the
candidate's in-engine scale/materials before allowing its mesh path to replace
the fallback. Keep the transient spawn disabled if the live capture shows
overlap with shoreline stones, vegetation, or player routes.
