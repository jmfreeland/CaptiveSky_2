# Tideglass sea-pea foliage candidate

Added a deterministic, collisionless Blender source candidate inspired by *Lathyrus japonicus* (sea pea), a native coastal plant. Its low radial runners, paired pinnate leaflets, tendrils, and small rose-pink flowers give it a different silhouette and palette from the upright marram and rush candidates. The model is a standalone cluster about 46 x 46 x 6 cm; the exported FBX contains 3,216 triangles across three material slots and no collision meshes.

Validation on 2026-10-18:

- Blender 5.2.2 built the asset from `Scripts/AssetPipeline/assets/tideglass_sea_pea.py`.
- Exported-FBX verification re-imported the mesh, found one UV layer and the three expected material slots, reported `PROBLEMS []`, and rendered `Saved/CaptiveSky/ComfyBlender/TideglassSeaPea/Preview.png`.
- The three source-contract tests pass. They check the stable candidate layout, separate material/flower/leaf structure, and absence of collision or random generation.
- The Blender preview shows a readable low, branching silhouette and distinct pink blooms. It is still a procedural source preview, not evidence that the color, scale, lighting, or density works on the Island.

No Unreal assets or map actors were imported or placed. Next review should import the FBX into a fresh, verified-isolated scratch project, check its material scale and readability against rock and grass at gameplay distance, then try a small patch on the dry/sheltered side of the Tideglass shoreline only if it complements the wet-ground palette. Do not increase global vegetation density based on this candidate.

## Flower-readability source pass (2026-10-18)

The first preview showed a useful low, spreading foliage silhouette, but the
flowers were difficult to distinguish from its leaves. The procedural petals
are now larger while keeping the two-flower-per-runner arrangement, material
palette, mesh bounds, and triangle count effectively unchanged. The disposable
rebuild at
`Saved/CaptiveSky/ComfyBlender/Codex_TideglassSeaPeaBloomReview_20261018/`
re-imported and verified cleanly: 47 x 47 x 7 cm, 3,216 triangles, three
material slots, one UV layer, no collision, and `PROBLEMS []`. In the new
Blender preview the rose-pink blooms read more clearly against the blue-green
leaflets. This is an improvement to candidate readability, not an Island-scale
art approval.

The complete source-only asset-pipeline suite passes **82 tests** using the
Codex-bundled Python runtime. The standard `python` launcher is unavailable in
this environment, and Blender's bundled Python lacks Pillow required by the
bread-texture test; neither limitation affects the successful full suite.
Unreal import, Island lighting/scale, and placement remain pending. Keep the
existing placement gate: test in an isolated Unreal project first, and only
consider a small sheltered Tideglass-edge patch if the plant complements the
actual wet-ground palette without adding global density.
