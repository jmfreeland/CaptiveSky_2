# Nanite foliage restart verification

## Result

The main `CaptiveSky_2` project was closed after the editor showed **All Saved**, then reopened in UE 5.8 from `D:/Projects - Athena/Unreal/CaptiveSky_2/CaptiveSky_2.uproject`. The Island loaded, and after texture streaming settled, the viewport again showed the ground-cover layer and several tree silhouettes. This confirms the project opens with the foliage visible after restart; it does not identify every tree mesh or prove a gameplay-performance improvement.

## Fresh restart verification (2026-10-29, 06:15–06:16)

After a second clean editor restart, the fresh `Saved/Logs/CaptiveSky_2.log` records `r.Nanite.ProjectEnabled:1`, `bEnableNaniteFoliage:1`, and `r.Nanite.Foliage:1` during startup. The editor then opened the Island map; its status bar showed **All Saved**, and the viewport displayed ground cover plus multiple tree silhouettes. This verifies the requested setting survived restart and the map loaded. It does not prove that each foliage mesh is Nanite-enabled or that Nanite improves performance.

A contemporaneous `nvidia-smi` sample reported 9,641 MiB / 12,282 MiB (78.5%) device memory in use and 54% GPU utilization. This is whole-device usage, not Unreal-specific allocation or a gameplay peak. The log also emitted the existing landscape edit-layer merge warning (4033x4033 requested versus 1024x1024 configured); the map still rendered, but warning cost and effect remain unmeasured.

## Evidence

- Latest fresh editor log: `Saved/Logs/CaptiveSky_2.log`, with startup at 06:15 and Island map initialization at 06:16 on 2026-10-29.
- Startup config reports `r.Nanite.ProjectEnabled:1`, `bEnableNaniteFoliage:1` (deferred dummy CVar), and `r.Nanite.Foliage:1`.
- The Island was visible in the main editor after loading and texture streaming completed. The editor status bar showed **All Saved**.
- A contemporaneous `nvidia-smi` sample reported 9,641 MiB / 12,282 MiB GPU memory in use (about 78.5%) and 54% GPU utilization. This is whole-device usage, not an Unreal-specific allocation or a gameplay peak.
- The latest log reports that landscape edit-layer merge wants a 4033x4033 batch while the configured maximum is 1024x1024. The landscape rendered, so this is a warning rather than a load failure; its cost/impact is not measured here.

## Limits and next checks

No PIE session was started during this restart, so this does not validate in-game FPS, peak VRAM, interaction, or the Nanite foliage assembly's visual equivalence. Given the already-high whole-device memory sample and earlier memory-pressure warnings, do not use this editor-open sample to infer safe PIE headroom. A meaningful performance claim still needs a controlled, matched daytime/nighttime assembly A/B with per-process peak memory and frame-time measurements, after addressing the current landscape merge warning if it contaminates the comparison.
