# Opt-in GPU memory preflight for rendered sessions — 2026-10-31

The workstation's RTX 4080 Laptop GPU reported 9,888 MiB used of 12,282 MiB
total (2,394 MiB free) while the Island editor was open. The device was at
78 °C and 55% utilization. `nvidia-smi` did not expose per-process allocations
under WDDM, so the device-wide total is a conservative launch signal, not an
Unreal-specific memory measurement. The editor log also reports a 4,096 MiB
texture-streaming pool, and an earlier PIE run displayed a video-memory
exhaustion warning. No new rendered session was started under this baseline.

`Scripts/Start-Spectator.ps1` now has the opt-in `-RequireGpuHeadroom` gate for
future real-RHI runs. It queries one selected NVIDIA device before project
output directories are created or Unreal is launched, and fails closed if
`nvidia-smi` is unavailable, returns malformed data, or reports less than the
configurable 4,096 MiB reserve (`-MinimumFreeGpuMemoryMiB`). `-GpuIndex` selects
the device. The gate is not enabled by default and does not change existing
launches; it does not monitor memory after startup or guarantee safe runtime
headroom. NullRHI runs do not use the rendered-session gate.

## Verification

- Four spectator launcher regression tests passed.
- `Scripts/tests/Test-GpuMemoryBudget.ps1` passed parser, reserve-boundary,
  under-budget and malformed-output checks.
- The live device snapshot parsed as 9,888 / 12,282 MiB, and the preflight
  rejected it with 2,394 MiB free, before launching a game or creating the
  requested output directories.
- The spectator launcher passed PowerShell syntax parsing.
- The open main D: Island project, its settings, source, and map were not
  changed. This guard exists only in the writable worktree until explicitly
  integrated into the main project.

After background graphics load is reduced and at least 4 GiB is free, use the
guard for a short, bounded daylight route first; continue to watch in-run
memory and frame rate, and stop immediately on memory pressure or a sustained
drop below the 30-FPS floor. Only then consider a 15-minute resident-feedback
session with isolated world data and the established request cap.
