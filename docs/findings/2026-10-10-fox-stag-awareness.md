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
- The first `CaptiveSky2.Visual.WoodlandFoxStagAwareness` D3D12 image was an
  Editor-world capture. Its pale upright Listening Stones were saved editor
  proxies, not the transient procedural stone forms used in Game; don't use it
  to judge runtime landmark art.
- The test now also runs in standalone Game context. On 2026-10-10,
  `CaptiveSky2.Visual.WoodlandFoxStagAwareness` passed there with D3D12, zero
  subject blockers, Aster's uncertain observation verified, and the live
  runtime stone presentation. The 1600x900 frame includes transient staged
  Aster, fox, and stag; it is a composition proof, not evidence of natural
  encounter frequency or a polished highlight. Pale placeholder-looking
  stones remain in the lower-left of the frame. The game loaded roughly 1.78M
  ground-cover instances, 14,864 trees, and 14,950 shrubs; its 30-FPS gate
  measured 36.24 FPS for five seconds. Thinking was disabled and model requests
  capped at zero. The playtest used an isolated data root and made no persistent
  map or world-state edit.

Evidence: `Saved/CompileScratch/Claude_Props/Saved/Logs/FoxStagAwareness_Final.log`,
`Saved/Logs/Codex_FoxStagAwareness_Composition_20261010.log`,
`Saved/Logs/Codex_FoxStagAwareness_Game_20261010.log`, the earlier Editor-world
frame at `Saved/Viewpoints/WoodlandFoxStagAwareness_/20261010_090640/01_Aster_Witnesses_FoxStagGlance.png`,
and the standalone Game frame at
`Saved/Viewpoints/WoodlandFoxStagAwareness_/20261010_092449/01_Aster_Witnesses_FoxStagGlance.png`.
