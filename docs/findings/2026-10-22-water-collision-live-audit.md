# Island water collision: post-reload live audit

## Result

After reloading `/Game/Maps/Island`, neither the hidden water plane nor the
`WaterBodyOcean` component is configured as a blocking collider in the editor.
The current live state therefore does not support the earlier simple diagnosis
that the hidden plane is what lets the player walk across the sea. A later
offshore PIE spawn did log the player entering surface swimming at Z=940 cm,
but no movement input or shore-return check was performed, so the reported
walking symptom is not yet fully reproduced or cleared.

## Live evidence

- The hidden plane is
  `/Game/Maps/Island.Island:PersistentLevel.StaticMeshActor_0`, using
  `/Water/Meshes/S_WaterPlane_256`. Its component is invisible, at relative Z
  940 cm, and scaled 2400x. Its `BodyInstance` uses the `NoCollision` profile
  and `collisionEnabled=NoCollision`.
- The mesh asset itself does contain a generated box (`QueryAndPhysics`,
  `CTF_UseSimpleAsComplex`). That asset-level shape is overridden by the placed
  component's `NoCollision` profile; it is not evidence of live blocking.
- `WaterBodyOcean_1.WaterBodyOceanComponent` is at relative Z 940 cm, has large
  water extents, but its `BodyInstance` also reports `NoCollision`. Its response
  table includes overlap responses for Pawn/WorldDynamic, which does not make a
  `NoCollision` body block movement.
- `Landscape_0` bounds are X/Y = -201,600..201,600 cm. A downward world trace
  at `(-101,300, 100,460)` hit land at about Z 2,684 cm. Traces from Z 2,000 to
  500 at `(250,000, 0)` and `(0, 250,000)`—outside the landscape bounds but
  within the water body's reported extents—returned no hit.
- The running editor still reports `r.Nanite.Foliage=0`; PIE is not running.
- The current session log query found no matching video-memory/texture-pool
  exhaustion warning. It did contain a material warning at 09:52 UTC:
  `M_Island_Textured_Wet: Requesting an invalid TextureIndex! (21 / 20)`.
  This is worth tracking separately from the user's earlier graphics-memory
  warning and should be checked against the current material rebuild.

## Follow-up verification

- Reloaded `/Game/Maps/Island` again in the open editor. Four downward traces
  from Z 2,000 to 0 at the cardinal points `(±250,000, 0)` and `(0, ±250,000)`
  all returned no hit. These points are outside the landscape bounds and
  inside the ocean's reported extents; this still does not substitute for a
  player-capsule movement test.
- `CaptiveSky2.Agent.IslandOcean` passed in the live editor (0.072 s, no
  warnings). This confirms its automated swim-state rules, not that a player
  can travel through water and return to shore in PIE.
- `CaptiveSky2.Agent.IslandWeather` passed (0.039 s), but emitted
  `No Island ground-cover instances placed`; therefore that fixture provides
  no evidence about the live Island's scatter or species mix.
- A post-reload viewport capture still looked sparse from the sampled camera
  angle. It showed conifer and broadleaf silhouettes, but was not a population
  audit and cannot establish how many tree species appear across the Island.
- A read-only GPU snapshot showed 8,020 MiB used and 3,978 MiB free on the
  RTX 4080 Laptop GPU while PIE was stopped. This stopped-state snapshot is not
  enough to claim that the memory warning or 30 FPS target is resolved.
- A later, deliberately brief in-editor PIE spawn at `(210,000, 0, 1,300) cm`
  produced `[PlayerWater] ... entered surface swimming at Z=940.0`. The session
  ran 45.9 real seconds because editor/tool latency exceeded the intended
  sub-30-second window; the play-safety log records 5 model requests, all
  completed, with no in-flight requests at shutdown. No movement input,
  capsule mode sampling, or shore-return check was performed.
- During that PIE sample, a GPU snapshot showed 11,006 MiB used / 992 MiB free
  (adapter reports about 11,997 MiB dedicated). PIE was stopped immediately;
  afterward it recovered to 6,080 MiB used / 5,918 MiB free. The session log
  has no video-memory exhaustion warning or D3D12 crash, but 992 MiB free is
  dangerously close to the earlier reported exhaustion condition. FPS was not
  measured, so this does not validate the 30 FPS target.

## Next diagnostic

The remaining water check is an input-driven trip back toward shore, with the
player capsule location, movement mode, floor-hit component, and WaterBody
overlap captured if walking persists. Prefer a zero-request diagnostic path:
the in-editor PIE sample used 5 model requests and briefly left under 1 GiB of
dedicated VRAM. First lower the foliage/texture residency or provide a session
cap that can be applied before PIE initialization. If the capsule is actually
supported, identify that blocking component/landscape collision; if it is not,
inspect the swim-entry/shoreline transition and visual height perception.
Avoid changing the WaterBody or plane collision again unless runtime evidence
identifies one as the blocker.

## Limits

This is editor-state evidence, not a gameplay reproduction. `trace_world` is a
single line trace, and no player capsule, swimming transition, or actual water
surface contact was observed. The landscape material warning is log evidence;
this audit did not rebuild or modify the material.
