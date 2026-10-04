# Intermittent viewpoint stalls: render-thread evidence

## Scope

UE 5.8.3, isolated scratch project with shared Content, editor-only viewpoint
captures: no PIE, resident turns, canonical memories, map saves, or persistent
rendering changes. These are short SceneCapture samples, not sustained gameplay
clearance. Shared Content can change independently; these runs are not a controlled
asset before/after comparison.

## Evidence

- `Saved/Logs/Codex_RenderProfileCold_20261004.log`: Wind Arch at 11:00 failed
  the existing gate, p95 throughput 27.65 FPS, 50/50 valid intervals. CSV
  `Profile(20261004_152456).csv` has 1142 data frames. An approximate measurement
  window (zero-based row 1090, count 50) has frame-time p95 35.44 ms and zero
  PSO/compute-PSO misses. The CSV and camera phases are not exactly synchronized;
  do not replace the harness result with this approximate window.
- `Saved/Logs/Codex_RenderProfileWarmPair_20261004.log`: first run failed at
  24.96 FPS p95; the second never reached a completed capture result. The
  180-second tick watchdog stopped and finalized the CSV. The previous wrapper
  failed to exit promptly; only its verified, owned editor PID was terminated.
  No passing warm comparison exists.
- That pair's CSV, `Profile(20261004_153022).csv`, contains 1785 data frames.
  Rows 1720..1779 cover loading/early capture, **not** a steady-state gate window.
  `Exclusive/RenderThread/PrepareDistanceFieldScene` reaches 38639.76 ms and
  totals 54750.64 ms over those 60 frames. Game-thread EventWait reaches
  38613.17 ms. FrameTime reaches 39190.18 ms. GPU BasePass p95 is 0.19 ms and
  volumetric clouds 1.38 ms in this window. GPU scopes are not summed into a
  purported critical-path total.
- `Saved/Logs/Codex_HighlightsFresh_20261004.log`: a later, unprofiled 17:00
  capture of all nine daytime cameras passed. Ground-level p95 throughputs were
  55.27..57.98 FPS; the survey recorded 30.51 FPS. Fresh screenshots are under
  scratch `Saved/Viewpoints/2026-10-04_154352_h17.0`, with three selected copies
  in main `Saved/Highlights/2026-10-04/154352/`.
- `Saved/Logs/Codex_RenderProfileExitVerify_20261004.log`: the explicit-quit
  wrapper exited normally (process exit 0) after CSV stop and cleanup. The
  17:00 Wind Arch **test failed**, at 2.31 FPS p95 / 17.36 FPS wall throughput,
  50/50 valid intervals. Profiling and unprofiled runs differ in instrumentation
  and cold-start conditions; the slowdown is not yet separated from profiler
  overhead. Exit 0 does not establish a passing test.

The distance-field scene preparation is a specific CPU-stall lead. It does not
identify a culprit mesh, prove HTTP timeouts causal, or justify globally disabling
distance fields. Passing screenshots do not erase the failed runs.

## Tools and safety

`Scripts/Profile-IslandViewpoints.py` runs the existing automation test with CSV
profiling, records monotonic wall gaps (engine tick deltas can clamp to 0.125 s),
and explicitly quits after profiler stop. `COMPLETE` means wrapper cleanup, **not**
automation success. Read every `Test Completed` result and confirm the CSV exists.

Launch a built isolated project with:

```text
UnrealEditor-Cmd.exe <scratch-project.uproject>
  -ExecutePythonScript=<repo>/Scripts/Profile-IslandViewpoints.py
  -RenderOffscreen -csvGpuStats -csvCompression=0 -unattended -nosplash -nosound
  -NoZen -DDC-ForceMemoryCache -ViewpointHour=11 -ViewpointOnly=WindArchOverlook
  -ViewpointGroundCover -ViewpointNoWorldState
```

Do not add `-TestExit`: the wrapper must stop the profiler first. Optional
`-ProfileRepeats=2` retains both test results, with a combined 180-second tick
watchdog. A blocked synchronous frame can delay that watchdog; use an external
four-minute process deadline and stop **only the process launched for this run**.
Forced termination can lose the profile. Keep the editor hidden on Windows.

The read-only PowerShell summarizer accepts a CSV path and explicit frame window:

```powershell
$result = & ./Scripts/Summarize-RenderProfile.ps1 -Path <profile.csv> -StartRow 1720 -Count 60
$result.Statistics | Where-Object Metric -match 'FrameTime|PrepareDistanceFieldScene'
```

StartRow is a CSV data-row index, not the global engine frame number. Percentiles
use nearest rank; time scopes are milliseconds, PSO scopes are counters. Repeated
headings receive suffixes, nonfinite values are excluded, missing metrics are not
reported as zero, and out-of-range windows throw. A local three-row fixture
verified percentile/sum calculation, footer exclusion, duplicate headings,
nonfinite exclusion, and invalid-window rejection.

## Next investigation

### Rejected runtime ablation

The first matched attempt was invalid as an ablation:
`Codex_DFPairBaseline_20261004.log` and
`Codex_DFPairNoDistanceFields_20261004.log` both report `r.DistanceFields=1`.
The latter explicitly says the variable is read-only. Their p95 throughputs
(36.45 and 53.12 FPS, both tests passed) therefore demonstrate variability with
distance fields **still enabled**, not a benefit from disabling them. Both
processes exited normally and the map SHA256 remained
`3D1F2432A3D01D6E88C9CDF3D1B81FC7AEA312E7B64500F444FB8F1957E10AA8`.
The baseline CSV `Profile(20261004_155114).csv` has 1143 frames; whole-capture
PrepareDistanceFieldScene max 4469.94 ms / sum 15875.45 ms. Whole-capture
statistics include loading/warmup and must not be treated as steady-state FPS.

The diagnostic flag now **verifies** the effective startup value and refuses the
run if it is not zero. For an actual process-only ablation, add both:

```text
-ProfileDisableDistanceFields -ini:Engine:[SystemSettings]:r.DistanceFields=0
```

This is a command-line configuration override, not an on-disk config change.
See [Epic's configuration override documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/configuration-files-in-unreal-engine)
and [distance-field variable reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-console-variables-reference).

### Verified startup ablation

`Codex_DFStartupAblation_20261004.log` verifies value 0, set by SystemSettingsIni.
It passed at 56.44 FPS p95 / 59.59 FPS wall throughput, 50/50 valid intervals,
then exited normally. Map fingerprint remained identical to the baseline above.
CSV `Profile(20261004_155512).csv` has 1251 frames: whole-capture
PrepareDistanceFieldScene max 0.05 ms / sum 0.92 ms, compared with baseline
4469.94 ms / 15875.45 ms. Game-thread EventWait max 1632.98 ms / sum 2335.05 ms;
FrameTime still reaches 6398.58 ms during the broader capture lifecycle.
Disabling distance fields did not remove every startup stall.

Visual inspection of baseline `2026-10-04_155141_h17.0` and ablation
`2026-10-04_155541_h17.0` Wind Arch screenshots shows brighter inn panels,
changed arch shading and a brighter rock in the ablation. That is a real quality
difference; do not quietly deploy the toggle. This single sequential comparison
supports targeting distance-field scene preparation, not a universal FPS promise
or identification of the asset responsible. Other asset fingerprints and OS cache
conditions were not exhaustively held constant.

`Codex_DFAblationReject_20261004.log` validates the guard: flag without startup
override reports effective value 1 and refuses before profiler/automation start.
The editor exited within the one-minute external deadline.

Audit per-mesh distance-field sizes/atlas updates next. Foliage already disables
CastShadow; the primitive distance-field-lighting flag is documented as effective
only when CastShadow is true, so toggling that flag alone is not an evidenced fix.

CSV command background: [Epic CSV profiler documentation](https://dev.epicgames.com/documentation/unreal-engine/csv-profiler?application_version=4.27).
