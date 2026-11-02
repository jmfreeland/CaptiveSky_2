# Wind Arch grass patching — 2026-11-02

## Change

Wind Arch Festuca used to replace every fourth eligible grass candidate based
on the scatter loop's `Index`. That made its placement depend on generation
order rather than location. Festuca now follows a deterministic smooth field
with 8 m cells and a 0.34 threshold, forming irregular local patches while
remaining a minority of the existing exposed-grass candidates. The change
does not add candidate traces, foliage instances, components, collision,
navigation, per-frame work, or asset dependencies.

The focused
`CaptiveSky2.Agent.IslandWeather.WindArchFestucaPatches` regression checks
finite-input handling, same-point determinism, a bounded selection share, and
neighbor coherence. A standalone reproduction of the production hash/value-
noise math over a 128×128 grid at 2.5 m spacing measured 25.9% selected points
and 86.5% same-selection neighboring pairs. Those are deterministic
distribution checks, not rendered evidence.

## Validation and limits

The current worktree's UE 5.8.3 `CaptiveSky_2` Game target compiled and linked
successfully after the code and test were added. The Editor target was stale
(01:54 versus the 05:54 Game binary), so it was rebuilt successfully with 12
actions capped at four parallel jobs. That did not clear automation startup:
the `-game` launch fails with a `BufferReader.h` bounds assertion, and a
subsequent `UnrealEditor-Cmd.exe` launch without `-game` exits before writing
its requested project log or isolated data root. Its crash report records a
null-address access violation in early engine startup, before test discovery.
The open D: Editor is a separate checkout and was left untouched. A subsequent
bounded Editor-Cmd smoke with only `-ExecCmds=Quit` also returned exit code 0
without creating its requested log or isolated data root, and produced no new
crash context, so it does not establish successful startup either. No visual,
gameplay, frame-rate, or VRAM improvement is claimed.

[`Scripts/Run-IslandAutomation.ps1`](../../Scripts/Run-IslandAutomation.ps1)
standardizes the non-`-game` Editor-Cmd route with a fresh isolated data root,
zero resident model requests, a 4-minute timeout, a 4 GiB system-memory floor
and monitor, and explicit successful-test-result verification. Its first
launches exposed the early crash; it now surfaces a recent crash report when
Unreal exits before creating the requested log. Do not repeat the same launch
recipe until the startup context changes or the underlying crash is diagnosed.

Next: once whole-device GPU headroom and a stable test/preview session permit,
compare the Wind Arch ground-level composition and per-species counts against
the previous generation-order distribution. Keep the 30-FPS and VRAM gates in
place, and revert the patch-field choice if it reads as a single broad patch or
an empty verge at the actual landmark scale.
