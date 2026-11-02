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
successfully after the code and test were added. The automation itself has not
run: an attempted bounded NullRHI Game launch exited during engine startup with
a `BufferReader.h` bounds assertion before project/test initialization. The
open D: Editor is a separate checkout and does not contain this selector; it
was left untouched. No visual, gameplay, frame-rate, or VRAM improvement is
claimed.

Next: once whole-device GPU headroom and a stable test/preview session permit,
compare the Wind Arch ground-level composition and per-species counts against
the previous generation-order distribution. Keep the 30-FPS and VRAM gates in
place, and revert the patch-field choice if it reads as a single broad patch or
an empty verge at the actual landmark scale.
