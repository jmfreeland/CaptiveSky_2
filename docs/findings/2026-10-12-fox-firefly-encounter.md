# A brief fox–firefly encounter (2026-10-12)

The saved Island placement puts the fox at the Wind Arch woodland edge, about
28 m from the night-firefly habitat at Tideglass. The fox can now notice an
unhidden firefly within 22 m, but only with unobstructed visibility, and then
pauses for one short look-around. This makes a brief encounter possible as both
species wander toward one another without making the fox seek or chase them.
The cue rearms after fireflies leave a 32 m local area and has an 8-second
cooldown. Moving, waking, or resting foxes are left alone; a resident's existing
quiet-observation cue can take priority over a firefly glance.

The check shares the fox's existing 0.35-second wildlife-presence interval. It
adds no actor, timer, persistent state, resident memory, or model request. The
reaction is transient and uses the fox's existing look-around animation. The
larger search radius is still local to this habitat and the visibility trace
keeps occlusion from producing through-obstacle glances.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target built and linked in the isolated
  `Saved/CompileScratch/Claude_Props` project.
- `CaptiveSky2.Agent.WoodlandFox` passed under NullRHI. Its regression stages a
  firefly at 18 m, verifies an intervening visibility blocker prevents a glance,
  then verifies unobstructed notice without movement/startle, no repeated trigger
  during the same visit, no reaction while resting, and a later glance after
  the firefly leaves and returns. Thinking was disabled, the model-request
  limit was zero, and test world data used an isolated root.
- The adjacent `CaptiveSky2.Agent.RavenPerch` regression also passed under
  NullRHI with thinking disabled and zero model requests.
- The separate real-RHI `CaptiveSky2.Visual.WoodlandFoxResidentGlance` scene
  capture passed in the same scratch project. It transiently initialized
  1,780,637 ground-cover instances, 14,727 trees, and 15,063 shrubs; the fox was
  visible, with no subject-ray blockers. This is a resident-observation image,
  not a firefly shot or a live-play encounter. The central landmark pillars
  still dominate the composition. A tighter/diagonal camera trial enlarged the
  pillars without clearing the frame, so it was discarded; the next visual pass
  should stage the wildlife pair in a cleaner woodland-edge patch.
- The earlier live-view capture hid the fox in tall grass and overlaid texture
  statistics. No map or Content asset was modified during these captures.
- `LogIslandForestFox` now emits only an opt-in `Verbose` line when a glance
  actually begins. In an isolated 5-minute D3D12 `-game -Spectator` run at
  20:00, the naturally spawned fox noticed a naturally spawned/wandering
  firefly once, at 20.6 m, about 110 seconds after the safety window began.
  The session ended normally after 300.1 real seconds with zero model requests;
  resident thinking and Python were disabled, and world-state used a separate
  data root. This proves the encounter can happen in the real Island setup, but
  one run is not enough to estimate its typical frequency, and the animation's
  visual readability during that live event remains unverified.
- A fresh `CaptiveSky_2` Game-target build could not complete because other
  shared, dirty source/test files call editor-only `GetActorLabel`. The fox
  source compiled in the Editor target, which then ran the map with `-game` for
  this probe; those unrelated files were left untouched.

Build log: `C:/Users/freel/AppData/Local/UnrealBuildTool/Log.txt`.
Automation log: `Saved/Logs/Codex_FoxFirefly_Automation_20261012.log`.
Raven compatibility log:
`Saved/Logs/Codex_FoxFirefly_RavenPerch_20261012.log`.
Rendered resident/fox capture log:
`Saved/Logs/Codex_FoxResidentGlance_20261012.log`.
Natural nighttime probe log:
`Saved/Logs/Codex_FoxFireflyNightProbe_Run2_20261012.log`.
Telemetry regression log:
`Saved/Logs/Codex_FoxFireflyTelemetry_Automation_20261012.log`.
