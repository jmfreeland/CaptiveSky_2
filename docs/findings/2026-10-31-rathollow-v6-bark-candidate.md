# RatHollow V6 bark-material candidate — 2026-10-31

## Candidate and verification

V6 keeps the V5 hollow geometry and collision but assigns a separate,
versioned bark texture pack under
`Scripts/AssetPipeline/assets/textures/RatHollow_BarkV6/`. Its builder stages
those files into the local ComfyBlender texture library only when absent; a
same-named destination with different content causes an error rather than
being overwritten. The shared Bark and V4 maps remain untouched.

Blender 5.2.2 built and re-imported the V6 FBX successfully: 2,352 triangles,
2.708 × 1.452 × 1.424 m, four material slots, five box colliders, one `UVMap`,
and `PROBLEMS []`. The entrance-facing neutral preview is at
`Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV6/Preview.png`. The color is
warmer and less pale than V5, but the hollow still reads as a stylized stump
with a nearly circular opening. The normal and ORM maps are copied from the V4
candidate, not derived from the V6 albedo; their old directional grooves still
dominate the surface. Treat V6 as a material comparison only, not a final asset
or an approved Rat home.

The new `test_rathollow_barkv6.py` pack regression and 25 existing RatHollow
source tests pass under Blender's bundled Python 3.11 (26 total). This validates
the versioned texture inputs and source geometry rules; it does not check the
texture under Unreal lighting, in woodland composition, or at Rat scale. The
full asset-pipeline suite attempted 108 tests but failed to import the unrelated
`test_bread` module because Pillow (`PIL`) is absent from this Python. That
missing dependency also prevents regenerating V6's normal/ORM maps with the
standard `pbr.py` script.

## Generated texture prompt

Built-in `image_gen` edited the existing bark albedo with this prompt:

> Use case: stylized-concept. Asset type: tileable game texture, bark albedo for a small fallen-log rat shelter. Input image: edit target; retain the role as a seamless bark albedo, not a render. Make the bark surface quieter, more naturally weathered, and less sharply striped while preserving irregular lengthwise wood grain. Flat texture sample only; aged fallen broadleaf log bark, close-up; realistic stylized PBR albedo, tactile but not photorealistically noisy. Seamless square tile with no focal features, scale suitable for a 2.7 m hollowed log. Muted mid-value charcoal grey-brown with restrained warm umber variation; broad broken bark plates, shallow fissures, subtle moss-free grain; reduce the existing black-white contrast and long parallel ridges. Preserve 2D texture layout and all four tileable edges; even neutral albedo only, no directional lighting, highlights, cast shadows, vignette, perspective, moss, leaves, wood cut ends, objects, text, logos, or watermark. Avoid very dark grooves, pale white streaks, strong stripe repetition, smooth cut timber, and glossy finish.

## Remaining gate

Regenerate the V6 normal and ORM maps from this albedo before production use;
the current Blender Python lacks Pillow and SciPy, so the standard local `pbr.py`
step could not run. After the active editor can be reviewed safely, import V6
only into a disposable folder and compare it against the existing hollow in
woodland lighting from the Rat's entrance and a human approach. Ground contact,
concealment, collision/nav traversal, and an exact site still need validation.
No Unreal Content, map, canonical wildlife-home record, or resident memory was
changed during this material test.
