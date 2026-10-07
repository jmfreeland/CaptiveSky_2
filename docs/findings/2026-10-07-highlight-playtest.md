# Bounded highlight playtest (2026-10-07)

## Run

- Standalone UE 5.8.3 Game process, 1600×900 captures, 11 viewpoint screenshots.
- Agent thinking and Python disabled; isolated world-state, shader working directory, and DDC.
- `-MaxRealtimeSeconds=180`, `-MaxModelRequests=1`; the session ended at 180.1 seconds with **0 model requests** and exit status 0.
- Runtime log: `Saved/Logs/Codex_HighlightGame_20261007.log`.
- Captures: `Playtests/Codex_HighlightCurrent_20261007/Screenshots/` (local, untracked playtest output).

## What the capture shows

- The inner meadow now reads as substantially vegetated at ground level; the Listening Stones view is the strongest broad landscape image in this batch (`007_Listening_Stones.png`).
- The shore approach remains a large, mostly bare brown slope with sparse, thin trees (`001_Shore_Approach.png`).
- Tideglass reads as an opaque cyan pool/blockout, and the Wind Arch's oversized, dark segmented pillars dominate the frame (`005_Tideglass.png`, `008_Wind_Arch_Overlook.png`). These are useful visual-priority references, not polished showcase shots.
- The Inn exterior is legible, but the common-room capture is obstructed by the staircase and heavy beam geometry (`002_Inn_From_Path.png`, `003_Inn_Common_Room.png`).

## Runtime evidence and limits

- A follow-up controlled standalone sample used the repeated Tideglass camera, 1600×900, and a 600-frame CSV profile delayed 45 real seconds after spectator startup. It ran for 10.19 seconds. `FrameTime` was 15.60 ms median and 26.10 ms p95 (about 64 fps median and 38 fps at the p95 frame-time boundary), with a 272.15 ms maximum hitch. This sampled window clears the 30-fps target, but the maximum hitch remains and this is not a sustained player-controlled PIE test. The log recorded 350 unprecached PSO-creation hitches before sampling; the sample recorded at most one generic PSO miss and no compute PSO misses. CSV: `Saved/Profiling/CSV/Profile(20261007_022938).csv`; summary via `Scripts/Summarize-RenderProfile.ps1`.
- The full standalone profiling session ended normally after 180.1 real seconds with 0 model requests. It does not replace the earlier PIE profiling result (8–13 fps with the large ground-cover population); investigate why the standalone and PIE measurements differ before declaring the performance issue solved.
- A read-only parent-chain audit traced 38 missing `InstancedStaticMeshes` flags to four plant masters, with no per-instance overrides. The three graph-valid bases (Festuca, Phalaris, Typha) are now enabled and saved; the follow-up audit reduced missing flags to 15, all in Rhododendron. The Rhododendron master remains unchanged because it has four unresolved function references. A post-fix standalone visual check stalled in `TurnkeySupport` before loading the Island, so the material render change remains unverified. Details, backups and logs are in [the plant-usage finding](2026-10-07-plant-instanced-material-usage.md).
- A `WaterBrushManager` export-load warning also appeared. Tideglass's visible blockout appearance needs an asset/world inspection before attributing it to that warning.
- Together, the screenshot and profiling runs validate capture, one warmed repeated-camera performance window, and the real-time/request caps. They do not validate resident behavior, interaction quality, the visible UE editor, or a sustained 30-fps minimum under player-controlled PIE gameplay.

## Next steps

1. In the editor, repair the plant material usage flags, save the affected asset instances, and recapture the same daytime viewpoints.
2. Profile PIE and standalone using the same resolution, camera, and warm-up window, recording actual frame-time statistics; reconcile the result with the earlier low-FPS observation.
3. Prioritize authored visual improvements for the bare shore, Tideglass water surface, and Wind Arch silhouette before using these frames as final highlights.

## Noon visual recheck (2026-10-07)

A second standalone UE 5.8.3 spectator capture used the saved Island viewpoints
at noon and 1600×900. Agent thinking and Python were disabled; the world-state
root and shader-working directory were isolated, the warm local DDC was reused,
and the session had a 90-second realtime cap and a one-request ceiling. It ended
normally after 90.1 seconds with **0 model requests**. Fifteen frames were
captured as the viewpoint route repeated before the cap. Runtime log:
[`Codex_TideglassVisualRecheck_20261007.log`](../../Saved/Logs/Codex_TideglassVisualRecheck_20261007.log); captures:
[`Screenshots/`](../../Saved/Playtests/Codex_TideglassVisualRecheck_20261007/Screenshots/).

The runtime log confirms that the Lively Tideglass material and four transient
shore-rock visuals were applied. In the frame, however, the pool still reads as
a smooth, opaque cyan basin rather than water with an obvious readable surface
pattern. The close ground-detail view is crowded by tall, overlapping blades;
the wide shore approach remains mostly bare with thin tree silhouettes, and
the Wind Arch still dominates with oversized high-contrast rock forms. The Inn
view remains blockout-heavy. These are art-direction observations from stills,
not material-state or performance measurements.

The same log contains a `WaterBrushManager` export-load warning (including a
missing `/Script/WaterEditor` import). Because the Tideglass runtime subsystem
also logs that its separate water material was successfully applied, this
warning alone does not explain the cyan appearance and should be investigated
separately. This run had no CSV frame sampling and does not resolve the PIE vs.
standalone performance discrepancy or visually validate dragonfly ripple
responses; the interactive editor was still absent from the desktop window
inventory.

## Raven portrait follow-up (2026-10-08)

Three standalone Game attempts used a tag-follow camera on the runtime Raven at
17:00, with 1600×900 captures, disabled agent thinking/Python, isolated world
data, a warm DDC, a 60-second realtime cap, and a one-request ceiling. Each
ended normally with zero model requests. The Raven controller logged the rigged
Crow mesh and visible state in every run, and the `Raven` tag consistently
resolved to `BP_Raven_Placeholder_C_0`; the target itself is therefore present
and camera-anchor resolution is not the failure.

- The [first wide composition](../../Playtests/Codex_RavenPortraitGolden_20261008/Screenshots/002_Golden_Portrait.png)
  keeps Aster at the far left and Raven as a tiny dark shape near the ground.
- The [closer view](../../Playtests/Codex_RavenPortraitClose_20261008/Screenshots/002_Close_Portrait.png)
  and [elevated view](../../Playtests/Codex_RavenPortraitElevated_20261008/Screenshots/002_Elevated_Portrait.png)
  are dominated by foreground grass; Raven is not legible in either frame.
- All three frames show the red texture-streaming pool warning (about 4.4–5.7
  MiB over budget), further disqualifying them as showcase captures.

Logs: [`golden`](../../Saved/Logs/Codex_RavenPortraitGolden_20261008.log),
[`close`](../../Saved/Logs/Codex_RavenPortraitClose_20261008.log), and
[`elevated`](../../Saved/Logs/Codex_RavenPortraitElevated_20261008.log). Do not
continue blind tag-follow framing from ground-level grass. Next, use a verified
supported elevated roost/branch or a clear circulation edge for the portrait,
and resolve the streaming-pool warning before calling a runtime frame a
highlight. These are failed composition experiments, not claims that Raven is
missing or that the runtime is crashing.
