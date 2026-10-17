# Tideglass shoreline plant composition study — 2026-10-18

## Purpose and method

The four local Tideglass plant exports had only been reviewed as isolated
Blender thumbnails. I made a deterministic, CPU-rendered composition study to
see how their silhouettes and scale read together beside a simple water edge
and rounded-shore-stone stand-ins.

The script at
[`Scripts/AssetPipeline/reviews/tideglass_shoreline_composition.py`](../../Scripts/AssetPipeline/reviews/tideglass_shoreline_composition.py)
reimports the existing FBXs and their manifest color palettes; it does not
modify any source asset. The study arranges 6 rock-samphire, 4 sea-thrift, 3
marram-grass, and 2 soft-rush clumps. Blender 5.2.2 Cycles CPU rendered the
frame with 20 samples in about six seconds. Local output:
[`TideglassShorelineComposition.png`](../../Saved/Playtests/Codex_TideglassPlantComposition_20261018/TideglassShorelineComposition.png)
and [`composition.json`](../../Saved/Playtests/Codex_TideglassPlantComposition_20261018/composition.json).

![Blender-only Tideglass shoreline composition study](../../Saved/Playtests/Codex_TideglassPlantComposition_20261018/TideglassShorelineComposition.png)

## What the frame suggests

- The four candidate silhouettes can share a shoreline: low branching samphire
  sits nearest the wet edge, pink sea-thrift makes a readable flowering seam,
  marram forms a drier rear clump, and soft rush can mark one separate damp
  pocket.
- The species are distinguishable at this camera distance in the flat
  manifest palettes. This is a useful comparison of exported geometry and
  relative scale, not a prediction of UE materials or game readability.
- The study is still visibly a mockup: the ground plane is uniform, the shore
  stones are generic low-poly stand-ins, several plant clusters remain evenly
  spaced, and the right side is sparse. Do not copy these transforms into the
  Island scatter.
- Sea-thrift's dark, crossing basal leaves remain prominent. Samphire's
  narrow 27 cm width makes individual clumps small; its grouping—not a single
  clump—carries the foreground read. Soft rush is distinct but still risks a
  reed-like silhouette.

## Decision and next gate

Keep the local FBXs outside `Content/` and do not save map placements from this
mockup. When a responsive Unreal editor is available, import rock samphire and
sea thrift to a disposable destination and compare them under the real Island
lighting and installed instanced materials. Only after the single-mesh
close/play-distance review should a sparse shoreline strip be tested. Check
shadowing, collision/navigation, landmark sightlines, and a matched
gameplay-scale frame-time sample; preserve the 30-FPS p95 screen and bounded,
zero-model-request play safeguards. The current editor/automation access does
not provide that UE validation, so this is not placement or art approval.

No `Content/` asset, Island map, runtime source, or persistent world state was
changed by the composition render.

## Curved-bank and sea-pea follow-up (2026-10-17)

The preview script now lays the same shoreline candidates over a shallow,
curving bank with a procedural wet-sand ribbon, subtly varied soil, textured
water, a broken line of nine shore stones, and a lower camera angle than the
first top-down study. A small sea-pea group was added to the upper bank to
compare its spreading pink-flowered silhouette with samphire, sea thrift,
marram, and soft rush. The deterministic report contains 21 review-only
instances across those five candidate species.

The updated render is
[`TideglassShorelineComposition.png`](../../Saved/Playtests/Codex_TideglassPlantComposition_Curved_20261017/TideglassShorelineComposition.png);
its transform report is
[`composition.json`](../../Saved/Playtests/Codex_TideglassPlantComposition_Curved_20261017/composition.json).
The curved wet edge and distinct plant groups read more like a habitat than the
first rectangular slab study, but the broad upper bank remains visually quiet
and the candidate materials remain palette stand-ins. This is a composition
direction aid only: the sea pea and all transforms remain unapproved for the
Island. Unreal lighting, ground contact, shadows, gameplay readability, and the
30-FPS p95 screen still require the bounded real-RHI review described above.
