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
existing Rat-scale RootArch V2 shelter with one collisionless Woodland Fern,
one Wild Strawberry groundcover patch, and a shallow straw threshold mat. The
inner timber material is warmer, and the two plant species are deliberately
asymmetric rather than arranged in a ring. It preserves the shelter's eight
collision boxes and entrance anchors; each plant placement keeps at least
25 cm of radial clearance beyond its conservative reach estimate. The
resulting FBX is 193 × 190 × 90 cm, about 11,700 triangles, twelve material
slots, one `UVMap`, and eight `UBX` collision pieces. Blender 5.2.2 re-import
verification reports `PROBLEMS []`; all 98 asset-pipeline source tests pass,
including the four focused composition/clearance tests.

The local FBX and entrance/approach previews are under the ignored path
`Saved/CaptiveSky/ComfyBlender/RatHollowMicrohabitatV1/`. These are
shape/material checks under simple lighting, not a placement or Unreal review.

## Decision and next gate

This is **not yet approved for import or placement**. The oblique preview now
shows a coherent low strawberry patch against the log, one fern peeking around
the opposite side, and straw inside the threshold; this is more habitat-like
than the first symmetric fern pass. The entrance-facing preview still shows a
stark cut end and dark tunnel, and neither preview proves that a
human-height approach will miss the home or that a Rat can traverse the
entrance in Unreal. Twelve material slots and the absence of LODs also need a
bounded in-engine cost check before placement.

Next, if an Unreal session becomes available, import this FBX into a disposable
review folder only. Compare true Rat-eye and human-height views at the two
previously surveyed North Woodland near-misses, then test collision/navigation
from the entrance threshold to the nest bed. Do not modify
`Agents/Agent_Rat_01/places.json`, `WorldState/Island.json`,
`IslandWildlifeHome.cpp`, or the saved Island until the visual concealment and
physical traversal gates both pass. If the fern sprays still look planted in
a row, reject this arrangement and solve the shelter with a broader, less
regular woodland-edge composition rather than relaxing the site gate.
