# Firefly glow variation

Fireflies now receive one of three restrained glow tints derived from their
home location. The selection is deterministic and does not consume the shared
random stream, so it cannot reshuffle wildlife spawning or change activity,
light intensity, saved state, or model-request budgets. The existing pulse
continues to scale the emissive color uniformly.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded on 2026-10-19 (16
  actions), including `IslandFirefly.cpp` and `IslandFireflyTests.cpp`. The
  three modified sources were hash-checked in the isolated
  `Codex_TideglassPollinator_20261019` scratch project before building. The
  main editor's loaded module was not rebuilt or replaced.
- Added `CaptiveSky2.Agent.FireflyGlowColorVariation` assertions for same-seed
  repeatability, bounded tint channels, and palette variety. The automation
  was not launched because the coordination notes prohibit another project
  process while the pre-init crash remains unresolved.
- Unreal MCP could not connect to `127.0.0.1:8000`. The editor was left open
  and untouched; in-world nighttime appearance remains to be reviewed.
