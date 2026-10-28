# Black Alder asset preview — 2026-10-28

## Evidence

After restarting the main `CaptiveSky_2` editor, its startup log confirmed both
`bEnableNaniteFoliage:1` and `r.Nanite.Foliage:1`. The read-only Content Browser
review found `/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/`
with skeletal variants A–D and PVE metadata. Variant A opened in the Skeletal
Mesh Editor with a readable, leafy mid-sized crown rather than bare branches.
The asset viewer reported 3,927 triangles, 11,783 vertices, three UV channels,
and two material slots. This is a promising visual candidate, not an Island
lighting, wind, Nanite assembly, instancing, or runtime-cost approval.

No asset, map, project setting, or saved level content was modified in this
inspection. The editor viewport still shows a large exposed brown mid-distance
and only a few narrow tree silhouettes in the captured framing; it is an editor
viewport, not a Game/PIE result.

## Likely visibility bottleneck

Current source configures the three broadleaf `UInstancedSkinnedMeshComponent`s
for 8,000 cm start / 12,500 cm end culling, while spruce uses 35,000 / 55,000
cm. The recent live scatter log counted 1,200 each of Norway maple, European
aspen and European beech across 381 of 500 groves, but those numbers do not
prove that their silhouettes persist into the distant woodland. The shorter
broadleaf range is a plausible contributor to the repeated thin distant tree
line; it is a hypothesis, not a demonstrated sole cause.

## Next validation gate

Use the existing disposable
[`Preview-InstancedSkinnedMegaplant.py`](../../Scripts/Preview-InstancedSkinnedMegaplant.py)
with Black Alder A at the same Island landmark, lighting and camera as the
existing Aspen/Hazel probe. Keep its no-map-save and zero-model-request
behavior. First confirm material color and a readable canopy in Island lighting;
then compare one ordinary skeletal component with one instanced-skinned
instance. Only after that should a bounded group be considered. If it passes,
trade a share of the existing 3,600 broadleaf population for the new species
rather than increasing total trees, and test mid-distance visibility against a
matched frame-time capture. Do not expand global ground-cover density to solve
this composition issue.
