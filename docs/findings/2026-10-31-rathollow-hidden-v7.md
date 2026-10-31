# RatHollow Hidden V7 local candidate — 2026-10-31

## Intent

The user's chosen concept for Rat's first home is a hidden fallen-wood hollow.
V6's entrance-facing render made the trunk read like a short upright stump, so
V7 adds a crooked entrance root and two long ground-hugging roots that clarify
the fallen log from an oblique visitor view. It retains the V6 versioned bark
maps and the original five-box collision layout. This is an asset candidate,
not a configured wildlife home.

## Build and verification

Built from `Scripts/AssetPipeline/assets/rathollow_hidden_v7.py` with Blender
5.2.2. The exported FBX re-imported at 2.743 × 1.513 × 1.424 m with 2,564
triangles, four material slots, five box colliders, and one `UVMap`. The two
named anchors (`entrance`, `nest_bed`) are within the mesh bounds. The verifier
reported `ZMIN -0.0321` and `PROBLEMS []`. The focused V7 source test and the
V6 texture-pack test both passed under Blender's bundled Python 3.11.

Local preview: `Saved/CaptiveSky/ComfyBlender/RatHollow_HiddenV7/Preview.png`.

## Visual assessment and limits

The oblique preview now reads as a horizontal fallen trunk. It remains a
stylized, high-contrast log; the entrance is only partly legible from this
angle, and the preview has no woodland dressing, terrain contact, Island
lighting, or Rat-scale view. A disposable moss-rock detail was removed after
it looked detached. V7 therefore improves the silhouette study but is not yet
approved for the Island.

No Unreal import, map edit, canonical `wildlife_homes` record, or Rat memory was
changed. Do not infer concealment, shelter, nav reachability, or safe capsule
clearance from this Blender export. Next review should compare the entrance
approach and a human visitor angle in a disposable Island composition, then
check actual ground contact and Rat traversal before creating the home record.
