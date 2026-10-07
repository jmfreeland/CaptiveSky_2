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
