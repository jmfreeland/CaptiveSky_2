# Tideglass layered foliage and dragonfly screen (2026-10-10)

## Scope

UE 5.8.3 real-D3D12 `CaptiveSky2.Visual.Viewpoints` editor-world captures at 1600×900 and 11:00 Island time. The full preview selected the three Tideglass viewpoints, added transient ground cover and daytime dragonflies, and passed `-NoWorldState`; it did not start a play session, let residents think, or issue model requests. Separate matched captures isolated the meadow/ground-plant layer and the woodland/tree-understory layer. All runs were sequential with the editor closed between runs.

## Results

The full preview placed 1,780,040 ground-cover instances. Its wet edge contained 7 Typha cattails and 3 Phalaris plants, with 559.6 cm minimum pool-edge clearance and 440.8 cm minimum shared spacing. The three daylight dragonflies spawned and were removed after the preview.

| Tideglass broad view (`02_Tideglass`) | p95 throughput | Wall-clock capture throughput | Result |
|---|---:|---:|---|
| Full transient cover + dragonflies | 28.45 FPS | 42.11 FPS | Failed the 30-FPS p95 screen |
| Meadow/ground plants only | 55.50 FPS | 55.96 FPS | Passed |
| Woodland (trees + understory) only | 58.11 FPS | 59.40 FPS | Passed |
| Meadow + trees (`meadowtrees`) | 56.05 FPS | 58.52 FPS | Passed |
| Meadow + understory (`meadowunderstory`) | 44.54 FPS | 52.39 FPS | Passed |

In the full run, the ground-detail and close-dragonfly views measured 57.75 and 58.79 FPS p95 respectively. The harness excludes only frame intervals over one second as automation stalls; it retained all 50 measured intervals for each view. The full-run automation failed only on the broad-view p95 assertion. The no-ground-cover capture completed, but this harness does not report frame intervals without its transient ground-cover preview, so it is a visual reference rather than a performance baseline.

The individual and two-layer screens all passed, while the full transient-cover preview did not. Meadow + understory was the slowest partial combination at 44.54 FPS p95; meadow + trees remained close to the individual-layer screens at 56.05 FPS. This narrows the diagnostic but does not explain the full-scene shortfall. All measurements were separate sequential runs, not an interleaved A/B, so workload order, thermal/cache state, and system activity were not controlled. Blender remained open during the later combined-layer screens; its measured GPU usage was 2% at one sample, but its activity during each capture is unknown. Treat the gaps as diagnostic evidence, not a causal result, and repeat a matched Game profile with Unreal as the only active graphics workload. These offscreen editor SceneCaptures do not prove a GPU, landscape-shader, or vegetation-overdraw cause; no GPU trace, standalone Game profile, or PIE traversal was taken. Do not attribute the result to the authored Substrate landscape material; its pixel-shader instruction count is a separate proxy and this test does not isolate landscape cost.

## Visual read and next checks

The full preview improves local richness: varied near-ground plants, the first Phalaris alongside Typha, and a visible dragonfly at close range. The broad frame still shows sparse brown mid-distance, an over-dense foreground edge, a white/blockout-looking pool surface, and proxy-like landmark stones. The close dragonfly is an obviously pale placeholder-style mesh. This is useful composition and performance evidence, not a finished highlight image.

Next, repeat the full-cover and `meadowunderstory` cases as a matched standalone Game profile (and separately in PIE) with other graphics workloads closed, then use a GPU trace before changing species counts or the landscape material. The editor preview is enough to flag the full scene for investigation, not enough to justify a specific performance fix. For appearance, give the wet-edge band a deliberate readable shape and replace or improve the dragonfly presentation before judging it at normal gameplay distance.

## Evidence

- Full preview log: [`Codex_Tideglass_SubstrateDragonflies_20261010.log`](../../Saved/Logs/Codex_Tideglass_SubstrateDragonflies_20261010.log)
- Full broad frame: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02_Tideglass.png)
- Ground detail: [`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02a_TideglassGroundDetail.png)
- Dragonfly close: [`02e_TideglassDragonflyClose.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02e_TideglassDragonflyClose.png)
- No-cover visual reference: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-10_164242_h11.0/02_Tideglass.png); log: [`Codex_Tideglass_Substrate_BaseNoCover_20261010.log`](../../Saved/Logs/Codex_Tideglass_Substrate_BaseNoCover_20261010.log)
- Meadow-only log: [`Codex_Tideglass_MeadowOnly_20261010.log`](../../Saved/Logs/Codex_Tideglass_MeadowOnly_20261010.log)
- Woodland-only log: [`Codex_Tideglass_WoodlandOnly_20261010.log`](../../Saved/Logs/Codex_Tideglass_WoodlandOnly_20261010.log)
- Meadow + trees log: [`Codex_Tideglass_MeadowTrees_20261010.log`](../../Saved/Logs/Codex_Tideglass_MeadowTrees_20261010.log)
- Meadow + understory log: [`Codex_Tideglass_MeadowUnderstory_20261010.log`](../../Saved/Logs/Codex_Tideglass_MeadowUnderstory_20261010.log); [frame](../../Saved/Viewpoints/2026-10-10_165853_h11.0/02_Tideglass.png)
