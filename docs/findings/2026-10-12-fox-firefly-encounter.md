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
- This validates state and line-of-sight gating, not the frequency of natural
  encounters or how the cue reads in rendered play. The prior rendered view hid
  the fox in tall grass; no map or Content asset was modified.

Build log: `C:/Users/freel/AppData/Local/UnrealBuildTool/Log.txt`.
Automation log: `Saved/Logs/Codex_FoxFirefly_Automation_20261012.log`.
Raven compatibility log:
`Saved/Logs/Codex_FoxFirefly_RavenPerch_20261012.log`.
