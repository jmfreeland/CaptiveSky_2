# RatHollow V5 local candidate recheck — 2026-10-31

## Verification

Rebuilt `Scripts/AssetPipeline/assets/rathollow_barkv5.py` with Blender 5.2.2
into the worktree's ignored `Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV5/`
folder. Exported dimensions are 2.708 × 1.452 × 1.424 m, with 2,352 triangles,
four material slots, and five simple box colliders. Blender re-imported the
FBX with one `UVMap`; its verifier reports `ZMIN -0.0321` and `PROBLEMS []`.
The entrance-facing preview uses the pipeline's
`ASSET_PREVIEW_DIRECTION=-1,0,0` option. The 25 RatHollow-related source tests
also pass under Blender's bundled Python 3.11.

## Art assessment and live-world gap

The preview makes the opening and straw bed easy to locate, but the shell still
reads as a short, manufactured stump: the high-contrast bark dominates, the
moss is bright and sparse, and the entrance remains close to circular. This is
not sufficient evidence to approve the asset or home site. It is a neutral
Blender preview only; it does not show ground contact, woodland concealment,
Rat-scale traversal, or Island lighting. See the ignored local preview at
`Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV5/Preview.png`.

The D: project contains the original RatHollow import and older review copies,
but not this V5 asset. Its current `WorldState/Island.json` has no wildlife-home
record, so no canonical Rat home is configured there; this file check alone
does not prove whether the Rat spawned in the running editor. No project
Content, map, world state, or resident memory was changed.

## Next gate

When the active project can be reviewed safely, import V5 only into a disposable
review folder and compare it with the existing RatHollow in a woodland
composition from both the Rat's entrance approach and a human visitor view.
Verify terrain contact, visibility/concealment, Rat capsule/nav traversal, and
the entrance anchor before selecting a site. Keep the canonical wildlife-home
record and the Rat's place memory empty until that review passes; the user's
choice of a hidden fallen-wood hollow specifies the concept, not an exact
coordinate.
