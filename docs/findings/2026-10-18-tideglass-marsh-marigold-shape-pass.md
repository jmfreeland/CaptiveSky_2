# Tideglass marsh-marigold shape pass (2026-10-18)

The previous exported-FBX preview read as a handful of flat paddle-like leaves
with small, weakly resolved flower heads. I adjusted the authored candidate
source to use a finer rounded leaf grid with a shallow petiole cleft, keep a
slight cup/ridge across the blade, and make the blooms fuller with eight broad
yellow petals and a more visible orange receptacle. A follow-up gives each
sepal a rounded longitudinal profile, shallow cupping across its width, and a
slight raised-tip/length variation so the flower heads read less like flat
umbrellas. This is still a stylized, collisionless Caltha palustris-inspired
wet-margin candidate; the change does not assert that it belongs at a specific
Island location.

## Verification

- Focused source-contract tests passed after the sepal profile pass: 3/3.
- The asset-pipeline source suite passed: 85/85.
- The Blender 5.2 scripted build exported and re-imported
  `SM_TideglassMarshMarigold_01` successfully. The verifier reported
  `PROBLEMS []`, one `UVMap`, four material slots, no collision shapes, and
  5,152 triangles. Bounds are approximately 75.6 x 77.7 x 39.1 cm.
- The re-imported preview was rendered with Cycles CPU at 24 samples while
  Unreal was open. It shows a more visibly cupped/scalloped flower profile, but
  remains an isolated Blender preview; no Unreal import, in-world lighting,
  gameplay-distance readability, or Island placement was checked. The existing
  imported Content candidate and saved map were not touched.

Preview artifact (gitignored):
`Saved/CaptiveSky/ComfyBlender/TideglassMarshMarigold/Preview.png`.

## Next review gate

Before Island placement, import the FBX into a disposable Unreal review folder
and check its material-slot assignments, scale, one-sided foliage shading, and
appearance beside the actual wet-margin plants at player distance. Preserve the
existing candidate as an alternative until that review passes; do not increase
marigold instance counts to compensate for a weak silhouette.
