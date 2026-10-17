# Sea-holly dune candidate

## Intent and ecological basis

Add a distinct, spiny-leaved flowering species to the local flora candidate set.
JNCC's fixed-dune habitat account records sea holly (*Eryngium maritimum*) as
unusual flora on lime-rich grey dunes, and its Abermenai to Aberffraw Dunes site
account notes abundant sea holly on mobile dunes. These are regional habitat
associations, not proof that the species belongs at a particular Island
coordinate: [JNCC fixed dunes](https://sac.jncc.gov.uk/habitat/H2130/) and
[JNCC Abermenai to Aberffraw Dunes](https://sac.jncc.gov.uk/site/UK0020021).

## Candidate and validation

- Source: `Scripts/AssetPipeline/assets/coastal_sea_holly.py`.
- Deterministic Blender 5.2 geometry: five low, lobed basal leaves with paired
  spines, four upright flower scapes, and twenty tapered blue bracts per head.
- Exported FBX: `SM_CoastalSeaHolly_01.fbx`, 53 x 47 x 46 cm,
  approximately 1,256 triangles, three flat-color material slots, no collision.
- Blender re-import verifier: `PROBLEMS []`; one `UVMap`, all material slots
  retained, ground clearance 0.82 cm.
- Source contract tests: all three pass with Blender 5.2's bundled Python.
- Local preview: `Saved/CaptiveSky/ComfyBlender/CoastalSeaHolly/Preview.png`.

## Limitations and next step

The exported-mesh preview reads as a deliberately stylized, low-poly dune
plant; its pale material response and star-shaped heads may need art iteration.
It has not been imported into Unreal, checked against the Island landscape, or
reviewed at player distance. Do not place it or increase global foliage density
based on this Blender preview. The next gate is an isolated Unreal trial import,
then a small, ecologically plausible patch on a dry, open dune only if the
mesh, materials, scale, and appearance pass gameplay-scale review. Validate any
repeated-instance cost against the project's 30 FPS target before considering
broader distribution.
