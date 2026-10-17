# Coastal ling-heather candidate

## Intent

Introduce a small, locally authored Calluna-inspired flowering shrub as a new
species candidate for the dry coastal-heath transition. This adds a habitat
accent distinct from the existing wet-edge sedges and grass; it is not a
shoreline plant and has not been placed in the Island.

The habitat choice is deliberately narrow. JNCC's coastal dune heathland
description places heather in dry, acidic dune-heath communities, while its dry
heath description identifies *Calluna vulgaris* as characteristic vegetation.
These references support a dry-heath margin placement, not an exact Island
distribution or an ecological claim about every coastal site:
[JNCC coastal dune heathland](https://sac.jncc.gov.uk/habitat/H2150/),
[JNCC dry heaths](https://sac.jncc.gov.uk/habitat/H4030/).

## Candidate and validation

- Source: `Scripts/AssetPipeline/assets/coastal_ling_heather.py`
- Seeded Blender 5.2 geometry: six branching stems, small scale leaves and
  drooping mauve bells; three flat-color material slots, no generated textures.
- Exported FBX: `SM_CoastalLingHeather_01.fbx`, 45 x 54 x 39 cm,
  approximately 5,112 triangles, no collision.
- Exported-FBX verifier: no problems; one UV layer; all three material slots
  retained; ground clearance 0.15 cm.
- Source contract tests: three tests pass with Blender 5.2's bundled Python.
- Generated preview and index: `Saved/CaptiveSky/ComfyBlender/CoastalLingHeather/Preview.png`
  and `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.

## Limitations and next step

The clump is a procedural source candidate, not final art approval. Its preview
shows the exported mesh structure only. No Unreal import, material/lighting
review, LOD or repeated-instance performance test, landscape placement, or
resident interaction has been validated. Before adding it to the Island, import
it into an isolated trial folder, inspect it at gameplay scale in dry-heath
lighting, and measure a restrained scatter with the existing 30 FPS floor.
Keep the clump collisionless unless a specific gameplay use requires collision.
