# A quiet fox–stag moment (2026-10-10)

An awake fox within 7.5 m and clear sight can draw one brief look-around from
the grazing stag. It does not cause a chase, retreat, or other movement. The
cue is one-shot while the fox remains within 10.5 m and has an 18-second
cooldown; it rearms after the fox leaves that wider local area. Resting or
waking foxes, and resting, waking, moving, startled, or otherwise occupied
stags are not disturbed.

For a nearby resident that can see both animals, the existing situation summary
can report that the stag briefly lifted its head toward the fox. It says only
that the stag may have noticed the other animal; it assigns no motive or next
action. The moment adds no speech, memory, persistent world state, movement,
model request, or new tick: the stag checks at a half-second interval using its
existing actor tick and look-around animation.

## Verification

- The UE 5.8.3 isolated scratch editor target built and linked successfully.
- `CaptiveSky2.Agent.WoodlandDeer` passed with NullRHI, thinking disabled,
  zero model requests, and a 60-second watchdog. The regression covers an
  awake fox outside the notice radius, a blocked sightline, one visible glance,
  no movement/startle, the resident's uncertain observation, no repeated cue,
  rearming after the fox leaves the 10.5 m range, and both animals resting or
  waking without disturbance.
- The same test had an older Raven leave/re-approach assertion at 7 m, while the
  current shared Raven wildlife selector uses a 14 m forget radius. The fixture
  was aligned to the current 14 m behavior; the runtime range was not changed.
- No Game/PIE capture was made. This verifies the bounded state change, not
  natural encounter frequency or rendered readability at gameplay distance.

Evidence: `Saved/CompileScratch/Claude_Props/Saved/Logs/FoxStagAwareness_Final.log`.
