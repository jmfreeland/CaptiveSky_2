# Broader broadleaf coverage without more tree instances

## Change

`AIslandWeather::InitializeGroundCover` previously allowed a broadleaf grove to spend as many as 32 of its trees on one of the capped maple, aspen, or beech batches. The global quota of 600 per species was therefore consumed by a relatively small number of groves, leaving most of the Island dominated by spruce silhouettes.

The generator now limits broadleaf substitutions to five trees per grove. Remaining positions in each existing grove still use spruce. This keeps the total tree population and the per-species caps unchanged while spreading the three broadleaf silhouettes through more of the forest. A runtime log count reports how many accepted groves contain at least one broadleaf tree.

## Validation

- UE 5.8 Live Coding compile succeeded.
- After a clean reload, a request-disabled Simulate session regenerated 87,465 ground-cover instances and 14,430 total trees.
- The same run reported 600 Norway maple, 600 European aspen, and 600 European beech instances across 363 of 500 accepted groves; birch and hazel coppices remained at 39 and 57.
- No out-of-memory or texture-pool warnings appeared in the sampled D3D12/streaming logs.
- NVIDIA telemetry sampled 9,863 MiB of 12,282 MiB dedicated GPU memory in use. This remains high utilization; absence of a warning during this short run is not proof that longer play cannot exhaust memory.
- The 60-second session ended automatically at its cap with model requests set to zero. Safety defaults were restored to 1,800 seconds, 120 model requests, and continuous play disabled.

## Meadow-scatter follow-up

The first live daylight capture still showed a sharp lush/bare transition. The broad meadow pass used two hard cutoffs on its seeded habitat fields; those are now a soft 40–64% acceptance gradient, averaging 46% on a uniform field sample. This keeps the 96,000-candidate budget and 5% landscape-density setting unchanged while retaining a stronger tendency for lush communities.

- UE 5.8 Live Coding succeeded; `CaptiveSky2.Agent.IslandWeather` passed with no failures.
- A 30-second, zero-request Simulate regenerated 84,801 ground-cover instances from 8,317 valid terrain cells and 136,002 bounded traces. The prior baseline was 87,465 instances at the same configured budget, so this pass did not increase vegetation count.
- A fixed-angle live-world capture still shows broad bare-looking regions. The softened distribution is not yet a visual resolution; the next iteration should measure coverage by spatial cell and test a broader visitor-facing camera set before tuning density again.
- No out-of-video-memory message appeared during this short pass, but it did not capture a live GPU-memory telemetry sample and cannot establish safety for longer play.
- The Maple/Aspen/Beech missing-material-usage warnings remain in the editor log; foliage/Nanite appearance still needs validation after the in-flight material fix.
- The 30-second session ended automatically with zero requests. `MaxRealtimeSeconds`, `MaxModelRequests`, and `bContinuousPlay` were restored to 1,800, 120, and false.

## Limits and next checks

The runtime log proves broader spatial assignment, not that every tree's foliage material renders correctly or that the species read clearly from a visitor's view. The latest fixed-angle capture still shows patchiness; quantify per-cell cover next, then inspect additional visitor viewpoints. Measure steady-state frame time against the 30 FPS floor, repeat VRAM telemetry during a representative play session, and test movement across water with a possessed player before claiming those issues are resolved.
