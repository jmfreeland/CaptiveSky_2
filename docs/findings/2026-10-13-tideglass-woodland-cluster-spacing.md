# Tideglass spruce-grove spacing trial (2026-10-13)

The same 11:00 Tideglass view still showed a thin, uneven spruce skyline after
the shrub visibility-range trial. The grove layout already had 500 centers,
but enforced 34 m between centers while mature trees reached about 16 m from
each center. This trial reduced the minimum center spacing to 24 m without
increasing the number of groves or changing the per-grove candidate budget.
The intent is to make the existing silhouette read as a more continuous,
irregular woodland edge.

UE 5.8.3 editor-target build succeeded. The no-world-state, real-RHI
`-GroundCover` viewpoint run placed 14,537 spruce (8,782 saplings), 15,080
broadleaf shrubs, and 1,498 flowering rhododendrons in 500 groves after
51,316 bounded traces. Compared with the 34 m capture, spruce count was
slightly lower (14,727), so this was a layout change, not an instance-count
increase. Visual inspection of the paired `02_Tideglass.png` frames shows
more even tree coverage along the distant skyline, though the broad brown
mid-distance remains and the improvement is modest.

- 24 m treatment frame: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_175242_h11.0/02_Tideglass.png)
- 34 m comparison frame: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_173910_h11.0/02_Tideglass.png)
- Treatment log: [`Codex_TideglassWoodlandClusterSpacing_Viewpoints_20261013.log`](../../Saved/Logs/Codex_TideglassWoodlandClusterSpacing_Viewpoints_20261013.log)

Treatment SceneCapture throughput was 48.12 FPS wall-clock / 36.95 FPS p95
for the broad view and 55.23 / 41.22 for the ground-detail view. The previous
34 m broad-view p95 was 45.50 FPS, so this sequential capture is lower and
the denser silhouette may cost some rendering headroom. Both broad-view
results remain above the project's 30-FPS minimum, but they are point-in-time
SceneCapture measurements, not an interleaved gameplay benchmark or proof of
30-FPS PIE performance. Keep 24 m as a visual candidate; recheck in a matched
gameplay profile before further tightening. Do not add more groves to solve
the remaining brown band until its material/habitat composition is addressed.

The viewpoint automation still reports a failure only for the optional
`02e_TideglassDragonflyClose` view because no actor has the `TideglassDragonfly`
tag; the primary treatment frames were saved. No map asset, world-state file,
or model-request budget was changed.
