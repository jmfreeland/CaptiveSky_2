# Mixed broadleaf trees within each grove

## Change

Each mixed grove already selected a stable starting broadleaf species from its
seeded habitat field, but every broadleaf substitution in that grove reused
that same species. The generator now rotates from that starting species for
each broadleaf tree, so a nearby grove can show its resolved maple, aspen,
beech, and Black Alder mix rather than a single broadleaf silhouette.

Placement order remains deterministic. Missing assets and exhausted
per-species quotas still use the existing fallback search, and all existing
grove spacing, exclusions, broadleaf quotas, spruce fallback, and total tree
budget remain unchanged. The feature changes local composition, not population
size or foliage-memory demand.

## Validation

- The UE 5.8 scratch Editor target linked successfully (46 actions initially;
  the focused test update rebuilt and linked in 4 actions).
- `CaptiveSky2.Agent.IslandWeather.BroadleafMixing` passed under NullRHI with
  zero model requests. It covers all four starting species, two full cycles,
  and invalid input bounds. Log:
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

After the in-flight Megaplant material/PVE setup is available in the editor,
use a bounded zero-request daylight view near a broadleaf grove to confirm the
species render distinctly. Record the same run's frame time and VRAM, while
keeping the established 30 FPS floor and current population cap.
