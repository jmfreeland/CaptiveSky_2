# Tideglass ground-cover species mosaic trial (2026-10-13)

The established Tideglass overview uses existing Fab foliage and a fixed
placement budget, but its species selector changed meshes every 7 m. The small
cells read as fine-grained noise at the overlook. This trial widened only the
deterministic species-selection grid to 14 m, retaining all eight existing
ground-cover meshes, the same habitat/exposure rules, and the exact candidate
and placement budgets.

The UE 5.8.3 editor target built successfully, including the updated
`IslandEcologyTests.cpp`. A no-world-state, real-RHI 11:00 capture ran the
generator and placed 14,537 spruce (8,782 saplings), 15,080 broadleaf shrubs,
and 1,498 flowering rhododendrons in 500 groves after 51,316 bounded traces;
the counts match the preceding 7 m capture. Visual inspection of the same
Tideglass camera shows larger, clearer plant communities and more visible
negative space in the near meadow, rather than a blanket density increase.
The remaining brown mid-distance and editor-preview proxy water/landmarks are
still conspicuous; this is an incremental ground-cover composition improvement,
not a finished beauty pass.

- 14 m treatment: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_180953_h11.0/02_Tideglass.png)
- 7 m comparison: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_175242_h11.0/02_Tideglass.png)
- Treatment log: [`Codex_TideglassSpeciesMosaic_Viewpoints_20261013.log`](../../Saved/Logs/Codex_TideglassSpeciesMosaic_Viewpoints_20261013.log)

The treatment SceneCapture measured 48.92 FPS p95 at the broad view and
50.71 FPS at ground detail, versus 36.95 and 41.22 in the preceding 7 m
capture. Both views moved in the same favorable direction, but these were
sequential point-in-time captures, not an interleaved performance benchmark;
do not attribute the entire difference to patch size. The treatment clears the
30-FPS SceneCapture screen but does not establish Game/PIE performance.

The focused `CaptiveSky2.Agent.GroundCover` automation could not complete: two
launch attempts crashed at `SecondsSinceStart=0` with the project's recurring
unresolved startup access violation (`PCallStackHash`
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`), before the requested project log
or test began. The current capture path did execute and save its primary
frames; the viewpoint suite reports only the known missing optional
`TideglassDragonfly` actor tag. Keep 14 m as the visually better candidate
with matching placement counts, but rerun the automation after the project
startup failure is diagnosed. No map asset, world-state file, or model-request
budget changed.
