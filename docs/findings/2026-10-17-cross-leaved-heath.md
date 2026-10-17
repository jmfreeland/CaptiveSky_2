# Cross-leaved heath candidate

## Intent

Add a second, morphologically distinct heath species candidate beside the dry
coastal-heath ling-heather. Cross-leaved heath (*Erica tetralix*) is intended for
a damp, acidic transition toward mire, not the open wet shoreline.

JNCC describes Northern Atlantic wet heath as occurring on acidic, nutrient-poor
substrates such as shallow peat or poorly drained sandy soils. Typical vegetation
mixes cross-leaved heath with heather, grasses, sedges and Sphagnum; these heaths
also grade between dry heath and wet valley mire. This supports a transition
habitat concept for the Island, but does not establish exact local species
distribution: [JNCC wet heath with *Erica tetralix*](https://sac.jncc.gov.uk/habitat/H4010/).

## Candidate and validation

- Source: `Scripts/AssetPipeline/assets/coastal_cross_leaved_heath.py`.
- Seeded Blender 5.2 geometry: seven upright leaders, two lateral branches per
  leader, whorls of four leaves, and three pendant bells per shoot cluster.
- Exported FBX: `SM_CoastalCrossLeavedHeath_01.fbx`, 46 x 53 x 39 cm,
  approximately 5,026 triangles, no collision.
- Exported-FBX verifier: `PROBLEMS []`; one UV layer; all three material slots
  retained; ground clearance 0.24 cm.
- Source contract tests: three tests pass with Blender 5.2's bundled Python.
- Generated preview and index: `Saved/CaptiveSky/ComfyBlender/CoastalCrossLeavedHeath/Preview.png`
  and `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.

## Limitations and next step

This is an unplaced art candidate. The Blender preview shows the exported mesh
and simple materials, not Unreal shading, its landscape, or gameplay-scale
silhouette. No Unreal import, repeated-instance performance test, LOD check,
ecological scatter mask, or resident interaction has been validated. Import to
an isolated trial folder first; inspect from player height against damp-heath
ground, then test a deliberately limited HISM population against the project's
30 FPS floor. Keep collision disabled unless gameplay later calls for it.
