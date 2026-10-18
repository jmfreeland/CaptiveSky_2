# Rat hollow mouth refinement (2026-10-18)

## Change

The reversible `rat_hollow_rootarch_v2` source now shapes the visible outer
lip and the first three interior rings into a low root arch. The entrance is
36 cm wide by 42 cm high, begins 3 cm above the ground-contact origin, and
opens into the wider shelter. The entrance-center anchor moved from 38 cm to
24 cm so consumers can still use a meaningful portal point. The tunnel floor
stations follow the new mouth profile; the 16 x 24 cm Rat capsule retains 10 cm
of lateral clearance per side and about 18 cm of overhead clearance at the
mouth. The rest of the trunk, collision layout, and world placement were left
alone.

## Validation

- `python -m unittest Scripts.AssetPipeline.tests.test_rat_hollow_rootarch_v2` —
  7 tests passed, including mouth proportions, capsule margin, transition into
  the larger tunnel, and a source check that the visible lip uses the new shape.
- Blender 5.2.2 built and re-imported the FBX in a separate local review folder.
  Result: 192 x 118 x 89 cm, 928 triangles, eight UBX box colliders, one
  `UVMap`, five material slots, `ZMIN 0.0131`, and `PROBLEMS []`.
- An 8-sample CPU Cycles render from the entrance direction shows the smaller
  opening and bedding. This is an asset preview, not a real-time Unreal review.

## Review gate

The candidate is local at
`Saved/CaptiveSky/ComfyBlender/Codex_RatHollowRefined_20261018/RatHollowRootArchBurrow/`.
It has not been imported into Unreal, placed on the Island, checked against
human-height concealment, or exercised with the Rat's live capsule and navmesh.
Do not add a home location to `Agents/Agent_Rat_01/places.json` until a
real-RHI in-world review confirms ground fit, entrance readability at Rat eye
height, and an unobstructed approach. The existing candidate sites remain
unapproved.
