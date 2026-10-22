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

## Limits and next checks

The runtime log proves broader spatial assignment, not that every tree's foliage material renders correctly or that the species read clearly from a visitor's view. Previous daylight inspection still showed patterned ground-cover patches and wide bare areas. Capture a post-change daylight view, measure steady-state frame time against the 30 FPS floor, and test movement across water with a possessed player before claiming those issues are resolved.
