# Tideglass sea-pea foliage candidate

Added a deterministic, collisionless Blender source candidate inspired by *Lathyrus japonicus* (sea pea), a native coastal plant. Its low radial runners, paired pinnate leaflets, tendrils, and small rose-pink flowers give it a different silhouette and palette from the upright marram and rush candidates. The model is a standalone cluster about 46 x 46 x 6 cm; the exported FBX contains 3,216 triangles across three material slots and no collision meshes.

Validation on 2026-10-18:

- Blender 5.2.2 built the asset from `Scripts/AssetPipeline/assets/tideglass_sea_pea.py`.
- Exported-FBX verification re-imported the mesh, found one UV layer and the three expected material slots, reported `PROBLEMS []`, and rendered `Saved/CaptiveSky/ComfyBlender/TideglassSeaPea/Preview.png`.
- The three source-contract tests pass. They check the stable candidate layout, separate material/flower/leaf structure, and absence of collision or random generation.
- The Blender preview shows a readable low, branching silhouette and distinct pink blooms. It is still a procedural source preview, not evidence that the color, scale, lighting, or density works on the Island.

No Unreal assets or map actors were imported or placed. Next review should import the FBX into a fresh, verified-isolated scratch project, check its material scale and readability against rock and grass at gameplay distance, then try a small patch on the dry/sheltered side of the Tideglass shoreline only if it complements the wet-ground palette. Do not increase global vegetation density based on this candidate.
