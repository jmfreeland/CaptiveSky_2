# Island water collision: post-reload live audit

## Result

After reloading `/Game/Maps/Island`, neither the hidden water plane nor the
`WaterBodyOcean` component is configured as a blocking collider in the editor.
The current live state therefore does not support the earlier simple diagnosis
that the hidden plane is what lets the player walk across the sea. The reported
gameplay symptom still needs a bounded PIE reproduction; no PIE session was
started during this audit.

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

## Next diagnostic

Reproduce once in a short, request-capped PIE session at a verified water
coordinate. Record the player capsule location, movement mode, floor-hit
component, and WaterBody overlap at the moment walking persists. If the capsule
is actually supported, identify that blocking component/landscape collision;
if it is not, inspect the swim-entry/shoreline transition and visual height
perception. Avoid changing the WaterBody or plane collision again unless the
runtime capture identifies one as the blocker. Keep the existing play/request
limits and stop the run promptly if GPU memory approaches the known ceiling.

## Limits

This is editor-state evidence, not a gameplay reproduction. `trace_world` is a
single line trace, and no player capsule, swimming transition, or actual water
surface contact was observed. The landscape material warning is log evidence;
this audit did not rebuild or modify the material.
