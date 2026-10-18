# Wet-meadow meadowsweet candidate

## Ecological and visual intent

Common meadowsweet (*Filipendula ulmaria*) is native across Britain and
Ireland. The BSBI Fermanagh account describes it in periodically wet marshes,
fens, ditches, damp meadows, stream banks and moist woodland openings, while
noting it avoids prolonged waterlogging. Its upright stems can reach about
1.2 m; the long pinnate leaves and loose, irregular panicles of creamy-white
flowers provide a mid-story silhouette between low wet-margin flowers and
taller rushes. [BSBI Fermanagh species account](https://fermanagh.bsbi.org/filipendula-ulmaria-maxim).

The candidate is intended for a damp meadow edge or raised stream bank—not
submerged ground, deep stagnant water, or strongly acidic blanket bog. Its
flower cluster is deliberately open, leaving visible gaps through the pale
panicles instead of becoming a solid cream puff.

## Source and safeguards

`Scripts/AssetPipeline/assets/wetland_meadowsweet.py` authors five varied
upright stems with opposite pairs of toothed pinnate leaflets and staggered,
five-lobed cream florets arranged in small open clusters. Geometry and palettes
are deterministic; the asset has four plain material groups and no generated
texture, paid model call, collision, Unreal import, map placement, or
resident-memory seed. The source test checks those design constraints and the
expected plant structure.

This is an asset-pipeline candidate, not a claim that the Island's vegetation
has changed. Keep the FBX in local `Saved/` output until Unreal is responsive.
Then import it to a fresh isolated generated-asset folder, check foliage
materials, scale, bloom readability, ground contact and frame cost at gameplay
distance, and only consider a small damp-meadow trial if it adds a distinct
middle layer without hiding landmarks or worsening the 30-FPS p95 floor.

## Validation status

The four source-contract tests pass. The full asset-pipeline suite also passes
(82 tests). Blender 5.2.2 built and re-imported the FBX on CPU: 104 x 105 x
121 cm, 13,010 triangles, four material slots, one
`UVMap`, no collision meshes, 1.17 cm ground clearance, and `PROBLEMS []`.
The CPU preview is available at
`Saved/CaptiveSky/ComfyBlender/WetlandMeadowsweet/Preview.png`; it shows the
compound-leaf form and airy cream flower sprays, but is not evidence of Unreal
material response or gameplay-scale readability. Unreal review and placement
remain pending; the user's editor is currently nonresponsive and has been left
running to preserve its state.
