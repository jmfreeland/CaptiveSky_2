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

- A nearby player (within 240 cm horizontally and 180 cm vertically) causes one
  short hop away, with a seven-second cooldown.
- Hop arcs last 0.48 seconds and stay within 420 cm of the animal's original
  habitat. Ground destinations must pass a static-world trace and slope/height
  bounds.
- Ambient hops are checked infrequently; rain or night increases activity,
  while dry daylight remains quiet. At most two instances are created.
- All movement and actor lifetime are session-local; the saved Island and
  persistent resident state remain unchanged.

## Validation and limits

`Source/CaptiveSky_2/Agent/Tests/IslandCommonFrogTests.cpp` covers observer
limits, invalid inputs, activity ordering, and hop shape. C++ compilation,
passed in UE 5.8.3 using the source-only `Saved/CompileScratch/`
`Codex_CurrentSourceBuild_20261018` target. UHT generated the new actor header,
and both the frog actor and automation translation unit compiled and linked.

The targeted automation did not start: `UnrealEditor-Cmd.exe` crashed at
`StartOfEnginePreInit` with a null-address access violation before test
discovery. The report and log are in
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
