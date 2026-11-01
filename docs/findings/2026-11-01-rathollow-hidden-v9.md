# RatHollow Hidden V9 candidate

V9 is a reversible source-and-preview iteration on the hidden fallen-wood home.
It is not imported into Unreal or placed in the Island. V7 remains the structural
baseline; see the [V7 review](2026-10-31-rathollow-hidden-v7.md) and
[V8 review](2026-11-01-rathollow-hidden-v8.md).

## Changes and export

The scripted Blender source,
`Scripts/AssetPipeline/assets/rathollow_hidden_v9.py`, keeps V7's five box colliders,
four material slots, and named entrance/nest anchors. It removes the three exposed
entrance-tip groups, replacing them with a low canopy and short asymmetrical root
shoulders that stay behind the front plane; low side roots remain tucked within the
trunk's silhouette. The anchor locations are unchanged: `entrance` at `[-132, 0, 66]`
cm and `nest_bed` at `[-32, 0, 32]` cm.

Blender 5.2 headless export and FBX re-import verification completed successfully:

- Dimensions: 263 × 146 × 142 cm.
- Geometry: 2,510 triangles, four material slots, one UV layer.
- Collision: five box colliders, unchanged from V7.
- Verifier: `PROBLEMS []`; minimum Z approximately -0.032 cm.
- V9 source checks pass, including front-plane and side-clearance constraints.

The generated FBX, manifest, verifier log, and previews are local under the gitignored
`Saved/CaptiveSky/ComfyBlender/RatHollow_HiddenV9/` directory. The entrance and
three-quarter previews are Blender structural checks, not Unreal screenshots. The
entrance opening is visible and the added roots no longer project beyond the log's
front silhouette, though the bark remains strongly striated in the preview.

## Still required before placement

Review a disposable Unreal import for material appearance, ground contact, entrance
readability at Rat scale, collision, navigation, and Nanite/foliage compatibility.
No live map, object placement, character memory, or `wildlife_homes` data was changed
for this candidate.

## Copied-map review workflow

`Scripts/Preview-RatHollowV9Review.py` now provides three explicit Unreal Python
stages. `import` reads this checkout's verified FBX and imports it to a new
`RatHollowHiddenV9Review` content folder with a separate texture-library folder;
it refuses to overwrite an existing mesh and validates the imported five-box
collision report. `duplicate` copies `/Game/Maps/Island` to
`/Game/Generated/ComfyBlender/RatHollowHiddenV9Review/Maps/Island_RatHollowV9Review`
without loading or saving the source. `place` only works when that copied map is
already open, and saves only that copy while preserving the authored collision.

Placement has no default coordinate. It requires
`Saved/Playtests/Codex_RatHollowV9Review/site.json` in the active project, with a
candidate index and XYZ copied from an `Island.RatHollowSiteAudit` log under
that project's `Saved` tree, written within the prior 24 hours. The stage rejects
near-misses, nonmatching coordinates, a canonical Island viewport, and duplicate
review actors. It does not write the canonical wildlife-home record or resident
memory.

In the Unreal Python console, set
`CAPTIVESKY_RATHOLLOW_REVIEW_CHECKOUT_ROOT` to this source checkout and run
`Scripts/Preview-RatHollowV9Review.py` once per stage with
`CAPTIVESKY_RATHOLLOW_REVIEW_STAGE` set to `import`, then `duplicate`. Open the
copied review map manually. After a fresh qualifying site audit, create
`site.json` under the review data folder with `site_id`, `candidate_index`,
`location_cm` `[X,Y,Z]`, `rotation_degrees` `[Pitch,Yaw,Roll]`, and the
project-relative `audit_log` path. Copy the index and exact XYZ from one
qualified candidate line. Only then run the `place` stage. Do not use Site A/B
from the old trials as defaults.

Eight focused source tests pass for audit freshness/coordinate matching, required
collisions, copy-only map saving, and refusal to place on the source map; the
58-test `Scripts/tests` suite also passes. These are mocked/source checks only:
no Unreal import, map duplication, placement, or visual/navigation review has
been run. Once the editor's project checkout is confirmed and a current audit
produces a qualified site, run the three stages and inspect Rat-eye and visitor
views before deciding whether this candidate is ready for a home record.
