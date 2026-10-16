# Rat hollow art pass — 2026-10-16

## Candidate

The scripted Rat hollow source now exports a separate versioned candidate,
`RatHollow_WeatherSplitV3`, rather than overwriting either earlier local FBX.
The pass makes the end silhouette less regular, opens a wider torn bark break,
adds two split-rim fibers, and lifts and flattens the moss patches so they sit on
the trunk rather than disappearing inside it.

Local output (gitignored):
`Saved/CaptiveSky/ComfyBlender/RatHollow_WeatherSplitV3/`.

## Verification

- Blender 5.2.2 built and exported the FBX successfully: 2,072 triangles,
  2.708 × 1.452 × 1.419 m, five embedded box colliders, and wood/dirt/straw/moss
  material slots.
- The pipeline re-imported that exported FBX and rendered `Preview.png` from
  the export. It reported `UVMap`, five colliders, and `PROBLEMS []`.
- The reported Z minimum is -0.0321 m; keep the collision/ground fit under
  review when the asset is eventually tested in Unreal.
- No Content import, Island placement, map save, or resident home record was
  made. Neither the chosen Rat home site nor the current visual-editor startup
  blocker has been resolved.

## Art decision

This is a technical candidate, not approved final art. The revised silhouette
is more broken and the moss is visible, but the exported wood still reads as
plank-like grain and the moss as separate green pads in the neutral preview.
Do not place it on the Island yet. ComfyUI could not start because its local
environment lacks `filelock`. Two generated bark-albedo attempts also failed
an objective wrap check (mean absolute opposite-edge RGB differences of 23–33
on a 0–255 scale), so neither was copied into the project or used by this
candidate. The next art step is a genuinely tile-aware bark surface, followed
by a woodland-context review; keep the original and V2 exports unchanged until
then.
