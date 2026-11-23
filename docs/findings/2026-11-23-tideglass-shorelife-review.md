# Tideglass shore-life close view — 2026-11-23

## Capture setup

Added `Config/TideglassShorelifeReview.json`, a single fixed camera aimed at the
east shore point used by the first daytime tidepool crab. The standalone UE 5.8.4
Game session loaded the saved Island at noon with an isolated world-state root,
zero model requests, and a 60-second cap. It reached world-ready in 31.5 seconds
and ended normally after 60.2 seconds. Six frames are under
`Saved/Playtests/TideglassShorelifeReview_Aimed_20261123/Screenshots/`.

The initial pool-centred version established a nearby waterline and produced a
300-frame CSV sample. Frame-time p95 was 14.95 ms, Render Thread p95 15.88 ms,
and GPU p95 14.25 ms; frame-time maximum was 31.29 ms. This is a short fixed-view
Game sample, not PIE traversal or a hardware performance guarantee. The later
crab-aimed capture had no CSV profile.

## What reads in game

The pool-centred angle shows the animated surface and nearby life but misses the
crab. The crab-aimed angle resolves it at the expected shore spawn. The authored
crab mesh loads and its eye stalks, claws, and legs are visible, but broad
foreground leaves cover much of the shell and legs in frames 004–006. Its
in-game shell also looks considerably paler and smoother than the local Blender
round-trip preview. That difference may come from lighting/exposure or imported
material setup; the capture alone does not identify the cause.

The wider composition remains visibly blockout-like: exposed flat brown terrain
behind the pool, the large shiny oval water surface, and rough landmark forms.
This view does not justify adding more general foliage. A better next art pass
would keep a small sightline around the crab's two existing shore spawn points,
then compare the imported shell material to the source preview under the same
camera. Keep any plant clearance local to these two observation points.

The Game log also repeats existing `InstancedStaticMeshes` usage-flag fallbacks
for several rhododendron, marsh-marigold, and yellow-flag-iris materials. Those
warnings are separate from the crab mesh, and no authored foliage material was
changed. No Island map or canonical runtime data was changed.

## Evidence and limits

- Crab-aimed Game frame: `Saved/Playtests/TideglassShorelifeReview_Aimed_20261123/Screenshots/006_Shorelife__Close.png`.
- Pool-centred Game frame and profile: `Saved/Playtests/TideglassShorelifeReview_20261123/Screenshots/006_Shorelife__Close.png` and `Saved/Profiling/CSV/Profile(20261123_070113).csv`.
- Both capped sessions used an isolated data root and zero model requests. The crab-aimed session reached the Island and ended normally.
- This is a spectator capture. It does not verify player-controlled shoreline traversal, a player noticing or examining the crab, or resident feedback.
