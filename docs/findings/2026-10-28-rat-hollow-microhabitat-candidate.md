# Rat hollow microhabitat candidate — 2026-10-28

## Purpose

The Rat's canonical `wildlife_homes` list is still empty. The selected design
direction is a hidden fallen-wood hollow, but the existing site scan rejected
the North Woodland near-misses because no candidate met its five-point canopy
cover gate, and earlier stand-alone previews read as a bare log with a dark
round mouth. This candidate tests whether sparse local understory makes the
asset feel more like a small habitat without changing the Island or weakening
the entrance/traversal requirements.

## Candidate and checks

`Scripts/AssetPipeline/assets/rathollow_microhabitat_v1.py` composes the
existing Rat-scale RootArch V2 shelter with three collisionless Woodland Fern
clumps. It preserves the shelter's eight collision boxes and entrance anchors;
each fern placement keeps a measured 25 cm radial clearance beyond the
generator's conservative frond reach estimate. The resulting FBX is
219 × 219 × 90 cm, about 18,664 triangles, seven material slots, one `UVMap`,
and eight `UBX` collision pieces. Blender 5.2.2 re-import verification reports
`PROBLEMS []`; all 98 asset-pipeline source tests pass, including the four
focused composition/clearance tests.

The local FBX and preview are under the ignored path
`Saved/CaptiveSky/ComfyBlender/RatHollowMicrohabitatV1/`. The preview is a
shape/material check under simple lighting, not a placement or Unreal review.

## Decision and next gate

This is **not yet approved for import or placement**. In the entrance-facing
preview the ferns add a little habitat context, but they remain distinct
sprays, the trunk still dominates, and the tunnel reads dark. The composition
does not yet prove that a human-height approach will miss the home or that a
Rat can traverse the entrance in Unreal.

Next, if an Unreal session becomes available, import this FBX into a disposable
review folder only. Compare true Rat-eye and human-height views at the two
previously surveyed North Woodland near-misses, then test collision/navigation
from the entrance threshold to the nest bed. Do not modify
`Agents/Agent_Rat_01/places.json`, `WorldState/Island.json`,
`IslandWildlifeHome.cpp`, or the saved Island until the visual concealment and
physical traversal gates both pass. If the fern sprays still look planted in
a row, reject this arrangement and solve the shelter with a broader, less
regular woodland-edge composition rather than relaxing the site gate.
