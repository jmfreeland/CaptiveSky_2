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

In the full run, the ground-detail and close-dragonfly views measured 57.75 and 58.79 FPS p95 respectively. The harness excludes only frame intervals over one second as automation stalls; it retained all 50 measured intervals for each view. The full-run automation failed only on the broad-view p95 assertion. The no-ground-cover capture completed, but this harness does not report frame intervals without its transient ground-cover preview, so it is a visual reference rather than a performance baseline.

The layer-only captures suggest that neither layer alone explains the broad-view shortfall; the combined scene is a candidate to investigate. They were separate sequential runs, not an interleaved A/B, so workload order, thermal/cache state, and system activity were not controlled. A low-memory Blender process was observed after the captures; whether it was active during any run or using the GPU is unknown. Treat the gap as a diagnostic hypothesis, not a causal result, and repeat the comparison with Unreal as the only active graphics workload. This is not proof of a GPU, landscape-shader, or vegetation-overdraw cause: the passes are offscreen editor SceneCaptures, and no GPU trace, standalone Game profile, or PIE traversal was taken. Do not attribute this result to the authored Substrate landscape material; its pixel-shader instruction count is a separate proxy and this test does not isolate landscape cost.

## Visual read and next checks

The full preview improves local richness: varied near-ground plants, the first Phalaris alongside Typha, and a visible dragonfly at close range. The broad frame still shows sparse brown mid-distance, an over-dense foreground edge, a white/blockout-looking pool surface, and proxy-like landmark stones. The close dragonfly is an obviously pale placeholder-style mesh. This is useful composition and performance evidence, not a finished highlight image.

Next, keep the same broad camera and isolate the combined layers (`meadowtrees` and `meadowunderstory`) to see which combination crosses the 30-FPS screen. Then profile the chosen composition in standalone Game and PIE before changing species counts or the material graph. For appearance, give the wet-edge band a deliberate readable shape and replace or improve the dragonfly presentation before judging it at normal gameplay distance.

## Evidence

- Full preview log: [`Codex_Tideglass_SubstrateDragonflies_20261010.log`](../../Saved/Logs/Codex_Tideglass_SubstrateDragonflies_20261010.log)
- Full broad frame: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02_Tideglass.png)
- Ground detail: [`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02a_TideglassGroundDetail.png)
- Dragonfly close: [`02e_TideglassDragonflyClose.png`](../../Saved/Viewpoints/2026-10-10_164041_h11.0/02e_TideglassDragonflyClose.png)
- No-cover visual reference: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-10_164242_h11.0/02_Tideglass.png); log: [`Codex_Tideglass_Substrate_BaseNoCover_20261010.log`](../../Saved/Logs/Codex_Tideglass_Substrate_BaseNoCover_20261010.log)
- Meadow-only log: [`Codex_Tideglass_MeadowOnly_20261010.log`](../../Saved/Logs/Codex_Tideglass_MeadowOnly_20261010.log)
- Woodland-only log: [`Codex_Tideglass_WoodlandOnly_20261010.log`](../../Saved/Logs/Codex_Tideglass_WoodlandOnly_20261010.log)
