# Tideglass mid-distance broadleaf visibility trial (2026-10-13)

The 11:00 Tideglass viewpoint still shows the same broad brown mid-distance and
thin spruce silhouette seen in recent Game reviews. This trial tested whether
the existing broadleaf grove shrubs were simply disappearing too close to the
camera. `IslandShrubs` now fades from 110 m and culls at 320 m, compared with
the previous 80–140 m range. It uses the same 15,063 non-colliding HISM
instances in 500 groves; no plants were added, moved, or persisted.

The editor target built successfully in UE 5.8.3. Same-viewpoint offscreen
captures were made with `-ViewpointGroundCover -ViewpointNoWorldState` at 11:00:

- Baseline: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_172104_h11.0/02_Tideglass.png),
  [`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-13_172104_h11.0/02a_TideglassGroundDetail.png),
  log [`Codex_TideglassCurrentFrame_20261013.log`](../../Saved/Logs/Codex_TideglassCurrentFrame_20261013.log).
- 320 m treatment: [`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-13_173910_h11.0/02_Tideglass.png),
  [`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-13_173910_h11.0/02a_TideglassGroundDetail.png),
  log [`Codex_TideglassShrubRange320_Viewpoints_20261013.log`](../../Saved/Logs/Codex_TideglassShrubRange320_Viewpoints_20261013.log).

The treatment frame shows a little more low foliage at the distant grove bases,
but visual inspection confirms that the exposed brown band remains dominant;
extending the cull distance alone is not a woodland-depth solution. The broad
view's conservative SceneCapture p95 was 39.29 FPS at baseline and 45.50 FPS
with the treatment; the ground-detail view measured 45.12 and 50.31 FPS. These
are sequential point-in-time screens, not an interleaved gameplay benchmark,
so the higher values should not be attributed to the cull change. They show no
observed performance regression in this capture.

Both capture runs saved their primary frames, but `CaptiveSky2.Visual.Viewpoints`
reported failure for the separate optional `02e_TideglassDragonflyClose` view:
the current Island has no actor tagged `TideglassDragonfly`. A focused
`CaptiveSky2.Agent.GroundCover` launch also crashed before producing a project
log, so the updated cull assertion has not yet been run in automation. The
successful editor-target build and rendered treatment frame are the available
validation; neither proves 30-FPS gameplay on target hardware.

Keep the 320 m range as a low-cost visibility improvement, not as closure of
the composition issue. The next visual pass should add deliberate, irregular
mid-distance habitat structure using an already-proven in-game silhouette and
material. Do not scatter the rejected hazel or unverified Beech/Maple outputs
to fill this gap. No map asset, world-state file, or model-request budget was
changed by this experiment.
