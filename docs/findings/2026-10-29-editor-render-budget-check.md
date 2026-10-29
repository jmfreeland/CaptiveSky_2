# Editor render-budget check after Nanite Foliage restart

## Result

The main `CaptiveSky_2` project was reopened on `/Game/Maps/Island` in UE 5.8.3. The fresh editor startup log (`Saved/Logs/CaptiveSky_2.log`, 2026-10-29 03:19 UTC) sets `r.Nanite.Foliage:1` from project configuration, alongside `r.Nanite.ProjectEnabled:1`; this verifies the restart applied the project setting. The Island finished loading and the editor status bar showed **All Saved**.

An editor-viewport `stat streaming` observation showed the configured texture pool at 4096 MiB and no explicit over-budget banner in that view. This is not a PIE/Game pool measurement and does not contradict the user's recurring memory-exhaustion report. The startup log contains the landscape edit-layer warning: a 4033×4033 merge batch is needed while the current maximum is 1024×1024.

A whole-device `nvidia-smi` sample while the editor was idle reported 6,539 / 12,282 MiB GPU memory used, 5,459 MiB free, and 0% utilization. The figure is not attributed to Unreal per process. It is less constrained than the earlier 9,635 MiB sample, but the editor already occupies substantial shared GPU headroom and a prior runtime preview ended with a Windows memory-pressure toast.

## Limits and next step

No PIE/Standalone session was started in this check. There is no new evidence for gameplay FPS, p95 frame time, peak Unreal VRAM, water traversal, or Nanite assembly visual equivalence/performance. Do not infer that Nanite Foliage fixed performance from CVar activation or the editor viewport.

Next, reproduce the user's exact memory warning in a single isolated runtime session and capture `stat streaming` plus `ListTextures` after streaming settles. Compare the required pool and largest resident/non-streaming contributors, and investigate the existing `M_Island_Textured_Wet` invalid texture index with the landscape-material owner. Only run a matched frame-time/peak-memory A/B when the editor is closed and sufficient GPU headroom is available; keep the real-time and model-request caps explicit and low.
