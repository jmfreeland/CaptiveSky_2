# RatHollow RootArch V2 source and export verification

## Candidate

`Scripts/AssetPipeline/assets/rat_hollow_rootarch_v2.py` refines the fallen-wood
hollow with a low, irregular ground-entry arch, a shallow interior floor ramp,
trunk-axis bark UVs, and eight simple collision boxes. It keeps the source
generator as the reproducible authority; exported FBX/texture files remain in
ignored `Saved/` output.

## Checks completed (2026-10-18)

- The focused `test_rat_hollow_rootarch_v2.py` suite passed 7/7. It checks the
  lower-rim opening, 36 × 42 cm entrance, gradual floor ramp, nest bedding,
  trunk UV mapping, and an axis-aligned clearance proxy for the Rat's nominal
  16 × 24 cm capsule through the tunnel.
- Blender 5.2.2 re-imported the exported FBX from a disposable copy and
  reported dimensions 192 × 117.7 × 88.6 cm, 561 polygons, one UV layer, five
  material slots, eight UBX collision boxes, and the two manifest anchors.
  `verify_asset.py` reported `PROBLEMS []` and produced a re-import preview in
  `Saved/CaptiveSky/ComfyBlender/Codex_RatHollowRootArchVerify_4f31bc2634cf447489767ba8c037e146/`.
- The preview shows weathered bark, root-like lateral projections, moss, and
  straw bedding. The entrance preview still reads as a dark recess in this
  simple Blender lighting; this is not evidence of how the interior reads in
  Unreal.

## Compared with the currently referenced home mesh

`IslandWildlifeHome.cpp` still loads the original `SM_RatHollow_01` asset. Its
manifest reports 256 × 136 × 134 cm, about 1,836 triangles, four box colliders,
and tiled wood, moss, and straw PBR texture sets. RootArch V2 is 192 × 117.7 ×
88.6 cm, about 928 triangles, and uses the same three broad texture groups; it
adds a distinct hollow-wood/dirt interior and four additional collision boxes
for the stepped floor and closed back. The candidate is therefore smaller and
roughly half the geometry, but this does not establish a lower GPU texture
cost or better material response. Its lower 36 × 42 cm mouth is the central
design change and must be checked against the imported Rat body in Unreal.

All test scratch and the verifier's disposable copy stayed under ignored
`Saved/`. The saved Island, imported Content, resident place-memory, and map
placement were not changed.

## Remaining gate

This is source and FBX round-trip evidence only. The candidate has not been
imported into Unreal or traversed by Rat CharacterMovement. Before updating
`IslandWildlifeHome.cpp` or Rat place-memory, import it into a separate review
folder, inspect bark/interior materials from both sides in Island lighting,
and run a bounded physical entrance-to-shelter traversal plus nav/ground
clearance check. Keep the Island map and `Agents/Agent_Rat_01/places.json`
unchanged until those checks pass and a sheltered site is visually confirmed.

## Offline recheck (2026-10-22)

The focused source suite was rerun and passed 7/7. The saved export remains
present at `Saved/CaptiveSky/ComfyBlender/RatHollowRootArchBurrow/`: its FBX is
95,036 bytes, preview is 867,668 bytes, and manifest is 2,611 bytes. The bark
base-colour, normal, and ORM textures referenced as `../Library/Textures/...`
are present (2.3 MB, 2.3 MB, and 1.0 MB respectively). I inspected the current
preview again: the outer bark arch and lower bedding read, but the interior is
still a large, nearly featureless dark recess under this preview lighting. This
is a visual warning, not proof of a bad Unreal material or unusable mesh.

No Blender re-import was run in this pass: `blender.exe` was not found on PATH
or at the pipeline's documented default `BLENDER_EXE` path; other install
locations were not searched. The Unreal MCP endpoint also has no reachable
listener, so no isolated Content import or collision review was attempted. Keep
the candidate local and do not change the saved Island or Rat place-memory
until those material/readability and traversal gates can be run.
