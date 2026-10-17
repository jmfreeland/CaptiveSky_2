# Tideglass plant candidates: provisional review order — 2026-10-18

## Evidence inspected

I compared the exported-FBX Blender previews for the four local Tideglass
candidates. These are CPU-rendered mesh/material previews from
`Saved/CaptiveSky/ComfyBlender/`; none is an Unreal lighting, shoreline
composition, gameplay-distance, shadow, or frame-time review. The FBXs remain
outside `Content/` and none is approved for Island placement.

| Candidate | Export | Provisional reading |
|---|---|---|
| Rock samphire | 27 × 47 × 40 cm, 4,410 triangles, 3 material slots | Best first test for the rocky Tideglass margin: branching succulent foliage and small yellow umbels give a distinct form and palette. It is a candidate, not a placement decision. [Preview](../../Saved/CaptiveSky/ComfyBlender/TideglassRockSamphire/Preview.png). |
| Sea thrift | 83 × 70 × 43 cm, 3,846 triangles, 4 material slots | Useful pink flowering accent to compare beside samphire, but the Blender image has dark, crossing basal leaves; check instanced material and shadow behavior before using it. [Preview](../../Saved/CaptiveSky/ComfyBlender/TideglassSeaThrift/Preview.png). |
| Marram grass | 108 × 97 × 74 cm, 3,078 triangles, 3 material slots | Distinct wind-combed grass silhouette; more plausible for a drier, salt-sprayed coastal edge than the wet pool lip. Do not use it to fill the broad woodland mid-distance. [Preview](../../Saved/CaptiveSky/ComfyBlender/TideglassMarramGrass/Preview.png). |
| Soft rush | 50 × 50 × 91 cm, 4,928 triangles, 2 material slots | Tall and narrow; the preview risks reading as a uniform reed/bamboo clump. Defer until a shoreline-context test shows the culms read as soft rush. [Preview](../../Saved/CaptiveSky/ComfyBlender/TideglassSoftRush/Preview.png). |

The ranking above is an inference from the visible Blender silhouettes and the
documented habitat intent. It is not botanical, lighting, or art approval.
The candidates' individual measurements and pipeline checks remain in the
species findings: [rock samphire](2026-10-17-tideglass-rock-samphire.md),
[sea thrift](2026-10-17-tideglass-sea-thrift.md),
[marram grass](2026-10-17-tideglass-marram-grass.md), and
[soft rush](2026-10-17-tideglass-soft-rush.md).

## Next controlled review

When the existing Unreal editor is responsive, import only rock samphire and
sea thrift into a disposable generated-asset destination. Compare them as
single meshes under the same Island lighting, material setup, scale, shoreline
rock, and camera before testing a small 5–10-clump nonblocking strip. Check
instanced material support, color/readability at close and normal play
distance, cast-shadow contribution, collision/navigation, and a matched
gameplay-scale frame-time sample. Keep the Island map and persistent world
state unchanged until that review passes. Preserve the 30-FPS p95 screen and
the existing bounded, zero-model-request playtest safeguards; Blender
previews alone do not pass either gate.

No source, Content asset, map, or world state was changed for this review.
