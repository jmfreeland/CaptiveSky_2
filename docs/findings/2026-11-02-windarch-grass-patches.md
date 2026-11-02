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
successfully after the code and test were added. The Editor target was stale,
so it was rebuilt successfully with 68 actions capped at four parallel jobs.
An asset-backed scratch project was used for the bounded automation attempt;
its `Source` and `Config` resolve to this worktree, while its `Content` junction
reads the main D: project's assets. The test process exited before it could
load the map or report a test result, and no asset/package changes were made.
This did not clear automation startup:
the `-game` launch fails with a `BufferReader.h` bounds assertion, and a
subsequent `UnrealEditor-Cmd.exe` launch without `-game` exits before writing
its requested project log or isolated data root. Its crash report records a
null-address access violation in early engine startup, before test discovery.
The open D: Editor is a separate checkout and was left untouched. Retrying the
focused test through the guarded runner against the asset-backed scratch
project reproduced the same early null-address access violation (crash
`43B6D10E47427073AEC1DDB8D786ECC8`); no automation result was emitted. This
rules out the worktree's empty `Content` directory as the sole explanation,
but does not identify the startup fault. The scratch project and crash outputs
are under `Saved/CS58`; the run-specific log/data locations are under
`Saved/Logs` and `Saved/Playtests`. No visual, gameplay, frame-rate, or VRAM
improvement is claimed.

[`Scripts/Run-IslandAutomation.ps1`](../../Scripts/Run-IslandAutomation.ps1)
standardizes the non-`-game` Editor-Cmd route with a fresh isolated data root,
zero resident model requests, a 4-minute timeout, a 4 GiB system-memory floor
and monitor, and explicit successful-test-result verification. Its first
launches exposed the early crash; it now surfaces a recent crash report when
Unreal exits before creating the requested log. It accepts an optional project
path constrained to the worktree, while keeping test data/logs rooted in the
worktree. Do not repeat the same launch recipe until the startup context
changes or the underlying crash is diagnosed. Five Python regression checks
for the runner's argument restrictions, project-path guard, isolation,
memory/timeout guards, and success-result verification pass under Blender
5.2.2 Python; they validate the harness contract, not Unreal startup.

Next: once whole-device GPU headroom and a stable test/preview session permit,
compare the Wind Arch ground-level composition and per-species counts against
the previous generation-order distribution. Keep the 30-FPS and VRAM gates in
place, and revert the patch-field choice if it reads as a single broad patch or
an empty verge at the actual landmark scale.
