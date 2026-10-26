# Live Island source and ecology validation — 2026-10-26

## Result

The restarted UE 5.8 editor had loaded `/Game/Maps/Island`, but its project DLL predated the current `IslandWeather.cpp`. The editor viewport by itself was therefore not a reliable view of runtime foliage. A Live Coding compile of the current source succeeded; a short Simulate then exercised the Island's `BeginPlay` scatter without changing or saving the map.

The post-compile runtime log reported:

- 84,765 landscape ground-cover instances after bounded terrain traces;
- 299 flower instances, including five Tideglass pollinator pockets: two marsh-marigold and three yellow-flag iris clumps;
- 14,430 trees, 15,036 broadleaf shrubs, and 1,491 flowering rhododendrons;
- 1,200 each of Norway maple, European aspen, and European beech, spanning 381 of 500 woodland groves; the total tree population remains unchanged.

This contrasts with the pre-compile run from the stale module: it reported 600 of each broadleaf species and zero Tideglass pollinator sites. The source was already present in the workspace; the corrective action for this editor session was compiling it, not changing the source.

## Water and safety checks

- `r.Nanite.Foliage` read back as `1` after restart and remained `1` after Live Coding.
- The retained `/Water/Meshes/S_WaterPlane_256` component reports `bodyInstance.collisionEnabled=NoCollision` and profile `NoCollision`. A downward world trace at `(250000, 0)` found no blocking hit.
- `CaptiveSky2.Agent.IslandOcean` passed in the live editor (0.36 s, no warnings). `CaptiveSky2.Agent.IslandWeather` and `CaptiveSky2.Agent.IslandTideglass` also passed after Live Coding (0.09 s and 0.05 s). The weather test's temporary context-free fixture logs that it placed no ground cover; that warning is not representative of the saved Island, whose runtime counts are above.
- The map remained `/Game/Maps/Island`, PIE is stopped, and no map save was performed.
- One initial Simulate diagnostic ran 39.9 real seconds and reserved four resident model requests (three were still in flight at stop). This remained below the configured 1,800-second/120-request session cap. Two later post-compile visual checks ran 9.8 and 10.0 seconds, each with zero requests and zero in-flight work at stop.

## Visual read and next steps

The short live capture shows substantially more woodland and understory than the editor-only viewport, including ferns, wet-edge plants, and a resting stag. The captured time was night, so it is not yet a good showcase image. The editor viewport still gives a misleadingly sparse preview because the runtime scatter is initialized during play.

Next, capture the same grove in daylight or golden hour and compare representative Island viewpoints. Keep visual captures short and inspect the play log's configured real-time/request caps before any longer resident run. The water-plane fix is verified structurally and by a trace; player-controlled swim, shore return, and jump behavior remain a separate interaction check.
