# Seeded broadleaf habitat patches

## Change

Each mixed grove previously rotated through all four broadleaf species in a
strict sequence. That guaranteed variety in counts but made each small stand a
perfectly balanced mixture. Trees now select from the existing smooth, seeded
habitat field at their ground-trace location, producing loose local species
patches and more gradual transitions between stands.

Placement order remains deterministic. Missing assets and exhausted
per-species quotas still use the existing fallback search, and all existing
grove spacing, exclusions, broadleaf quotas, spruce fallback, and total tree
budget remain unchanged. The feature changes local composition, not population
size or foliage-memory demand.

## Validation

- The UE 5.8 `CaptiveSky_2Editor` target compiled and linked successfully from
  this isolated worktree, including `IslandWeather.cpp` and the updated test
  source (4 incremental actions).
- `CaptiveSky2.Agent.IslandWeather.BroadleafHabitatSelection` covers local-field
  species selection, nearby patch consistency, repeatability, and a species
  transition across the landscape. It passed under UE 5.8 NullRHI with PCG/PVE
  disabled for the isolated test invocation. The contentless worktree still
  logged missing Island asset references, and its MCP plugin could not bind
  `127.0.0.1:8000` because the main editor already owns that endpoint; the
  pure test completed successfully and a follow-up MCP query confirmed the
  main editor remained connected. No Island map or resident agents were
  loaded. Log:
  `Saved/Logs/Codex_BroadleafHabitatSelection_20261029_retry.log`.
- The earlier `CaptiveSky2.Agent.IslandWeather.BroadleafMixing` pass validates
  the retired strict-rotation behavior only. Earlier log:
  `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_BroadleafMixing_20261029.log`.
- The pre-existing composite `BroadleafDiversity` test is currently red on two
  Black Alder cull expectations (80/125 m expected, 120/220 m configured),
  followed by missing Procedural Vegetation plugin asset warnings in the
  scratch test project. Those cull assertions and the separate visibility
  trial were not changed here.
- No PIE or rendered preview was started after the memory-pressure warning.
  Local visual readability, 30 FPS, and peak VRAM remain unverified; inspect a
  real mixed grove and check the per-species runtime counts before calling the
  user-facing “one kind of tree” concern resolved.

## Next

After this source change is integrated into the runtime project, use a bounded
zero-request daylight view near several broadleaf patches to confirm that the
spatial variation reads naturally and the species render distinctly. Record
the same run's frame time and VRAM, while keeping the established 30 FPS floor
and current population cap.
