# Timestamped weather context for headless residents — 2026-10-16

Raven's Discord gateway previously had no factual weather input. The Island
already persisted ground wetness, but not a timestamped atmospheric sample, and
the gateway only summarized the resident's own lasting creations.

When Unreal next saves world state after the environment subsystem has sampled
a live weather actor, `WorldState/<Map>.json` now records the observation time, rain,
ground wetness, cloud cover, mist, wind speed, and horizontal wind components.
The local sample is taken near the active player viewpoint when available, or
near the weather actor when there is no player. It is written into the existing `environment` object; it does not
change the map, trigger a save by itself, make a model call, or add work to the
per-frame tick. Older world-state files remain readable. Files without a live
weather sample continue to expose only their already-recorded wetness.

Headless replies receive the latest timestamped sample as public Island
context. The prompt explicitly says it is historical rather than live, tells
the resident to admit it cannot know current weather from Discord, and labels
the wind vector as map-relative—not a compass bearing. Only the resident's own
nests, cairn contributions, and stone-work title/intent remain private to that
resident; the shared weather sample does not reveal those contributions.

## Verification

- UE 5.8.3 scratch Editor target built successfully with 23 actions. The build
  compiled `IslandWorldStateSubsystem.cpp` and `IslandDayNightTests.cpp` into
  the scratch project's separate binaries; the user's open editor was left
  untouched.
- `CaptiveSky2.Agent.DayNightPersistence` passed under NullRHI in the scratch
  project. Its assertions confirmed that Unreal wrote an observation timestamp
  and rain, cloud, mist, and wind fields into the world-state file. The test
  used a transient weather actor and deleted its isolated fixture afterward.
- The gateway checks built and passed, including timestamp formatting,
  map-relative wind, explicit historical wording, shared weather visibility,
  and continued privacy of other residents' creations.
- No live Island session was started, so no project `WorldState` data was
  modified. The gateway will not have weather context until the next normal
  Unreal save after a live environment sample.

## Next

After the next ordinary Island save, verify its timestamped local environment
snapshot appears in the headless gateway context. Restart the gateway normally
so replies use the updated compiled assembly; test one Discord weather question
through the usual user-initiated message path.
