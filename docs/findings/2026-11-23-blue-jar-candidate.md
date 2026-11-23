# Blue Jar prop candidate — 2026-11-23

## Candidate

Added `Scripts/AssetPipeline/assets/blue_jar.py`, a dedicated 22 × 22 × 27 cm
stoneware jar with a deep blue glaze, restrained pale slip rings, a small side
handle, and one convex collision hull. It is separate from the Island map so it
can be inspected before the promise feature depends on it.

The Blender pipeline exported `SM_BlueJar_01.fbx`; Blender re-import verification
reported one `UVMap`, two material slots, the expected dimensions, one collision
shape, and `PROBLEMS []`. The first preview looked faceted, so the body and trim
were increased from 48 to 72 radial segments, the smoothing angle was raised, and
the glaze was brightened and made less reflective. The revised local `Preview.png`
reads as a smoother, clearer blue vessel. It has 5,452 source triangles.

The first Unreal import remains at
`/Game/Generated/ComfyBlender/BlueJar/SM_BlueJar_01.SM_BlueJar_01`. UE 5.8 refused
an in-place mesh reimport, so the revised mesh was imported into the separate
local review folder `/Game/Generated/ComfyBlender/BlueJar_Refined_20261123/`.
Its report measured 21.7 × 21.6 × 26.9 cm, one convex collision shape, and
`validation_errors: []`; its two imported material graphs contain the expected
revised base-color values.

## Limits and next step

The optional texture-packing stage did not run because the default Python
interpreter lacks NumPy. The jar therefore uses simple imported color and
roughness values without generated texture maps. Transient Unreal captures at
Roost_West and ListeningStones showed deep local shadows and still did not provide
a reliable finish review; an experimental key-lit isolated capture overexposed to
white and was discarded. No valid Unreal material/lighting review has passed, and
the prop has not been placed. Project Content and generated packs are local-only;
the tracked asset script is the reproducible source. Before placement, get a valid
Unreal material preview, then connect the jar to a real resident delivery/carry
action rather than treating a spoken promise as proof that an object moved.
