# Meadow flower population cap (2026-10-10)

The fixed Tideglass preview had a deterministic cap in its in-progress
`CaptiveSky2.Visual.Viewpoints` contract: between 251 and 512 Fab meadow flowers
and between 1,501 and 2,100 total flowers when combined with woodland
rhododendrons. The previous 512 candidate pockets yielded 1,003 meadow flowers
plus 1,487 rhododendrons (2,490 total), so the scene and test disagreed.

The candidate pool is now 256 sites; the reserved Listening Stones annulus still
uses its existing first 160 candidate sites. With the saved Island loaded, the
11:00 transient preview placed 295 meadow flowers, including 21 around the
Listening Stones, and 1,487 rhododendrons (1,782 total). The bounded trace pool
used 320 traces. Grass, shrubs, trees, wetland species, collision/navigation,
and persistent world state are unchanged.

UE 5.8.3 `CaptiveSky_2Editor` compiled successfully. The real-D3D12
`CaptiveSky2.Visual.Viewpoints` test passed with `-Only Tideglass
-TideglassDragonflies -GroundCover -GroundCoverSoloLayer meadowunderstory
-NoWorldState`; the three dragonflies were transient test fixtures and were
destroyed after capture. Its 30-FPS interactive gate recorded 38.28 FPS for five
seconds. SceneCapture p95 frame throughput was 42.07 FPS at the overview, 42.88
FPS at ground detail, and 51.68 FPS in the dragonfly close-up. These are
offscreen editor-world, layer-isolated results—not standalone Game, PIE, or
target-hardware performance guarantees. The overview retains a dense near
foreground, a sparse middle distance, and editor-only fallback water/landmark
proxies; use it to judge layer composition, not final runtime materials.

Evidence: [`Codex_Tideglass_MeadowUnderstory_20261010_Final.log`](../../Saved/Logs/Codex_Tideglass_MeadowUnderstory_20261010_Final.log),
[`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-10_094139_h11.0/02_Tideglass.png),
[`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-10_094139_h11.0/02a_TideglassGroundDetail.png),
and [`02e_TideglassDragonflyClose.png`](../../Saved/Viewpoints/2026-10-10_094139_h11.0/02e_TideglassDragonflyClose.png).
