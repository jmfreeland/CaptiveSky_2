# A brief fox–firefly encounter (2026-10-12)

An awake, idle Woodland Fox that notices an unhidden firefly within 4.5 m now
pauses for one short look-around. It does not chase the light, change its
foraging destination, or disturb the firefly. The encounter rearms after
fireflies leave a 7 m local area and has an 8-second cooldown. Moving, waking,
or resting foxes are left alone; a resident's existing quiet-observation cue
can take priority over a firefly glance.

The check shares the fox's existing 0.35-second wildlife-presence interval. It
adds no actor, timer, persistent state, resident memory, or model request. The
reaction is transient and uses the fox's existing look-around animation.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target built and linked in the isolated
  `Saved/CompileScratch/Claude_Props` project.
- `CaptiveSky2.Agent.WoodlandFox` passed under NullRHI. The regression stages a
  nearby firefly and verifies a short glance without movement/startle, no
  repeated trigger during the same visit, no reaction while resting, and a
  later glance after the firefly leaves and returns. Thinking was disabled,
  the model-request limit was zero, and test world data used an isolated root.
- The adjacent `CaptiveSky2.Agent.RavenPerch` regression also passed under
  NullRHI with thinking disabled and zero model requests.
- This validates state and animation dispatch, not how often natural Island
  placements bring the fox and fireflies together or how the cue reads during
  rendered play. No map or Content asset was modified.

Build log: `C:/Users/freel/AppData/Local/UnrealBuildTool/Log.txt`.
Automation log: `Saved/Logs/Codex_FoxFirefly_Automation_20261012.log`.
Raven compatibility log:
`Saved/Logs/Codex_FoxFirefly_RavenPerch_20261012.log`.
