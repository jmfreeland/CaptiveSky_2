# Blue Jar prop candidate — 2026-11-23

## Candidate

Added `Scripts/AssetPipeline/assets/blue_jar.py`, a dedicated 22 × 22 × 27 cm
stoneware jar with a deep blue glaze, restrained pale slip rings, a small side
handle, and one convex collision hull. It is separate from the Island map so it
can be inspected before the promise feature depends on it.

The Blender pipeline exported `SM_BlueJar_01.fbx`; Blender re-import verification
reported one `UVMap`, two material slots, the expected dimensions, one collision
shape, and `PROBLEMS []`. The local `Preview.png` reads as a compact blue vessel.
The Unreal content-only import report lists `/Game/Generated/ComfyBlender/BlueJar/SM_BlueJar_01.SM_BlueJar_01`, measured bounds of 21.7 × 21.6 × 26.9 cm, one convex collision shape, and `validation_errors: []`.

## Limits and next step

The optional texture-packing stage did not run because the default Python
interpreter lacks NumPy. The jar therefore uses simple imported color and
roughness values without generated texture maps. The preview is not an Unreal
lighting review, and the prop has not been placed in the Island. Project Content
and the generated pack are local-only; the tracked asset script is the
reproducible source. Before placement, inspect the imported mesh in Unreal under
inn lighting, then connect it to a real resident delivery/carry action rather
than treating a spoken promise as proof that an object moved.
