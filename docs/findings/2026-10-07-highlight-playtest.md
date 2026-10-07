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

- The log reports 400 unprecached PSO-creation hitches during initial rendering. The first screenshot took longer to complete; subsequent viewpoint intervals advanced about 680–745 frame-counter steps per ~12 seconds. This suggests roughly 57–62 fps after warm-up, but is only a rough estimate from log frame counters—not a controlled performance benchmark. It does not replace the earlier PIE profiling result (8–13 fps with the large ground-cover population); investigate why the standalone and PIE measurements differ before declaring the performance issue solved.
- Startup logs warn that plant material instances lack `InstancedStaticMeshes` usage and will use the default material in game. Warnings include Rhododendron, Typha, Festuca, and Phalaris materials. This is a concrete asset-side follow-up: fix/re-save the affected instances and confirm their rendered appearance in an interactive editor/game view.
- A `WaterBrushManager` export-load warning also appeared. Tideglass's visible blockout appearance needs an asset/world inspection before attributing it to that warning.
- This run validates screenshot capture and the real-time/request caps only. It does not validate resident behavior, interaction quality, the visible UE editor, or a sustained 30-fps minimum under player-controlled gameplay.

## Next steps

1. In the editor, repair the plant material usage flags, save the affected asset instances, and recapture the same daytime viewpoints.
2. Profile PIE and standalone using the same resolution, camera, and warm-up window, recording actual frame-time statistics; reconcile the result with the earlier low-FPS observation.
3. Prioritize authored visual improvements for the bare shore, Tideglass water surface, and Wind Arch silhouette before using these frames as final highlights.
