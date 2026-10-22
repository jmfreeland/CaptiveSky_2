# Island runtime validation after reload — 2026-10-22

## Scope

Reloaded `/Game/Maps/Island` in the existing UE 5.8.3 editor, verified runtime vegetation generation in Simulate, sampled `stat unit`, then ran a bounded player PIE at a known offshore location to check the water transition. No map or source files were changed or saved during this check.

## Current runtime evidence

- The reloaded Island generated 87,465 ground-cover instances, 14,430 trees (8,745 saplings), 15,036 broadleaf shrubs, and 1,491 rhododendrons across 500 groves.
- The tree mix was 600 Norway maples, 600 European aspens, 600 European beeches, 39 birch coppice, and 57 hazel coppice. The log reported broadleaf trees in 363/500 groves. The in-editor daylight Simulate view visibly contained multiple broadleaf forms, not only spruce; this is a single viewpoint, not a whole-island scatter review.
- In two short Simulate viewport samples, `stat unit` showed 20.97 ms and 20.02 ms frame time (about 48–50 FPS). Game thread was about 19.2–19.5 ms. Reported memory was about 8.9 GB and VRAM about 9.2 GB / 10.97 GB. This clears the 30-FPS target in that editor viewport only; it is not a standalone or long-run benchmark.
- A player PIE spawned at (500000, 500000, 1440), 50 m above the offshore surface. The runtime log recorded `BP_ThirdPersonCharacter_C_0 entered surface swimming at Z=940.0`. The viewport showed the character at the water surface. This supports the fix for the hidden collidable plane at that open-water location; it does not validate horizontal travel, wave-follow visuals over time, jumping, or shore return.
- During player PIE, `stat unit` reported 18.44 ms frame time, 20.54 ms game thread, 6.07 ms GPU time, 9.60 GB memory, and 10.25 / 10.97 GB VRAM. The brief run emitted no observed OOM, but VRAM was around 93% of the reported budget, so I stopped rather than attempting a longer traversal.

## Outstanding rendering warnings

The runtime still warns that the Norway maple, European aspen, and European beech bark materials lack the `InstancedSkinnedMesh` usage flag and fall back to a default material. A Nanite Assembly asset warning also says Nanite Assemblies may not appear as expected while Nanite Foliage is disabled. The material-fix work is tracked separately in `COORDINATION.md`; avoid editing those assets or its builder while that claim is active.

## Safeguards and next steps

The diagnostic session used a 60-second real-time ceiling and zero model requests. PIE was stopped after the check, and the class defaults were restored to 1,800 seconds, 120 model requests, and continuous play disabled. No user-facing Island package was modified.

Next, resolve the claimed foliage material usage issue and reduce the high VRAM footprint before a longer player-controlled test. Then verify horizontal swim input, wave following, jump exit, and return to walkable shore; repeat a 30-FPS check at a representative player view and longer steady state. Do not infer that the entire Island is naturally scattered from this single viewport.
