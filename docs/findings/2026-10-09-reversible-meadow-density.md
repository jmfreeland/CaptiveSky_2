# Reversible broad-meadow density control (2026-10-09)

The broad, terrain-wide meadow scatter now has an opt-in density scale:
`CaptiveSky.Island.GroundCoverLandscapeDensity`, default `1.0`, clamped to
`0..1`. A startup override such as
`-CaptiveSky.Island.GroundCoverLandscapeDensity=0.65` scales only the broad
landscape meadow candidate budget. Existing dense landmark clearings, the
Tideglass wet edge, flower accents, and woodland placement keep their own
counts and rules. The default preserves the authored scene; this is a
reversible performance control, not a replacement for the density/variety art
pass.

## Validation

- UE 5.8.3 scratch editor build succeeded after the source change.
- `CaptiveSky2.Agent.GroundCover` passed with `NullRHI`, resident thinking and
  Python disabled, zero model requests, and isolated world data. The regression
  verifies that the control is registered and can be lowered/restored.
- Two otherwise matched 1600x900 standalone Game captures used the saved
  Tideglass camera, 11:00 Island time, 600 CSV frames, a 90-second real-time
  ceiling, disabled thinking/Python, zero model requests, and isolated world
  state. In the default run, the log reports 1,779,869 meadow instances
  (`budget 1,920,000; density 1.00`); at `0.65`, it reports 1,158,892
  (`budget 1,248,000; density 0.65`). The Tideglass wet edge remained at 10
  plants in both runs.
- Full-window FrameTime p95 fell from 31.35 ms to 21.25 ms (about 31.9 to 47.1
  FPS); mean fell from 21.60 ms to 16.28 ms. The warm-window p95 values were
  31.20 ms and 20.83 ms. Maximums were 322 ms and 269 ms, so this fixed-view
  comparison does not establish a moving-play hitch or PIE guarantee.
- The matched frame remains compositionally similar: the foreground and
  landmark verges are intact, while the broad middle distance still reads
  sparse and brown. Lower density creates performance headroom but does not
  solve the landscape's ecological-composition problem. Keep the default at
  `1.0` until a broader visual/performance pass chooses a scene-wide setting.

Evidence: [`Codex_GroundCoverDensityAutomation_20261009.log`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_GroundCoverDensityAutomation_20261009.log),
default profile [`Profile(20261009_192845).csv`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Profiling/CSV/Profile%2820261009_192845%29.csv)
and [frame](../../Saved/Playtests/Codex_GroundCoverDensity_20261009/Screenshots/001_Tideglass.png),
65% profile [`Profile(20261009_193406).csv`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Profiling/CSV/Profile%2820261009_193406%29.csv)
and [frame](../../Saved/Playtests/Codex_GroundCoverDensity_20261009_65b/Screenshots/001_Tideglass.png),
runtime log [`Codex_GroundCoverDensityGame_65b_20261009.log`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_GroundCoverDensityGame_65b_20261009.log).
