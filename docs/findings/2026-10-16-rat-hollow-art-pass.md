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

This V3 remains a technical candidate, not approved final art. The revised
silhouette is more broken and the moss is visible, but the exported wood still
reads as plank-like grain and the moss as separate green pads in the neutral
preview. Do not place it on the Island yet. The notes above record the initial
ComfyUI startup failure and two earlier rejected bark attempts.

## V4 bark-texture candidate — 2026-10-16

The local ComfyUI server was still running on `127.0.0.1:8188`; its
`SeamlessTile` node was available, so no dependency install or network-exposed
listener was needed. The first new four-image batch looked like cracked stone
and tangled roots and was rejected. After changing the prompt to emphasize
weathered oak bark with mostly vertical ridges, candidate 6 was selected from
the second batch. The other generated images remain only in the local ComfyUI
output folder.

The selected 1024² albedo is preserved under the ignored
`Saved/CaptiveSky/ComfyBlender/Library/Sources/rat_hollow_bark/source.png`.
`pbr.py` produced local albedo, normal and ORM maps; these are separate from the
existing shared wood texture, use a 1.2 m tile size, and feed the new
`RatHollow_BarkV4` export. The generated map files remain under ignored
`Saved/CaptiveSky/ComfyBlender/Library/Textures/`. The reusable prompt graph is
`Scripts/AssetPipeline/workflows/lib_bark.json`; the selected image was
processed with `pbr.py --normal-strength 2.0 --delight 0.8`, then packed as
`RatHollow_Bark` with `pack_library.py`.

Verification: Blender 5.2.2 exported 2,072 triangles at 2.708 × 1.452 ×
1.419 m with five box colliders. Re-importing the FBX reported `UVMap`, all
four material slots resolved, five colliders, and `PROBLEMS []`; the floor
minimum remains -0.0321 m. Candidate 6's mean RGB difference across opposite
edges was 18.48 horizontally and 11.09 vertically, versus mean adjacent-pixel
differences of 18.63 and 11.12 respectively. This is a useful wrap diagnostic,
not proof of seamless appearance at every scale.

The local `Preview-Entrance.png` shows the hollow and straw bed; the oblique
preview shows stronger continuous bark-like ridging than V3, though the grain
is still quite directional and may read as weathered wood at close range. V4
is therefore the better preview candidate, not approved final art. It has not
been imported into Unreal, placed on the Island, or reviewed in woodland
lighting/gameplay. Keep all existing imports and Island state untouched until
that contextual review.

## V5 moss-conforming candidate — 2026-10-16

V5 keeps the V4 log, entrance, straw nest and collision layout, but replaces
the five detached moss-rock pads with thin irregular patches projected onto
the trunk's curved outer profile. The V4 source remains the default; the
versioned `rathollow_barkv5.py` builder produces a separate local export.

Blender 5.2.2 exported and re-imported the FBX successfully: 2,352 triangles,
2.708 × 1.452 × 1.424 m, five box colliders, `UVMap`, four resolved material
slots, and `PROBLEMS []`. The surface-following patches read as more attached
to the trunk in the entrance and oblique previews ([entrance](../../Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV5/Preview-Entrance.png),
[oblique](../../Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV5/Preview-Oblique.png)).
The bark still has strong directional contrast, the moss remains conspicuously
bright, and the export's minimum Z is -0.0321 m. This is an art/structure
candidate only: it has not been imported into Unreal, checked against a real
terrain surface or rat capsule, or approved for the canonical home. Keep V4
and the current Unreal import untouched until a bounded Island review can
compare them in woodland lighting from both rat-eye and human approach views.
