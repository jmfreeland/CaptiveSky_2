# Rat hollow: live Island audit

The requested home is a **hidden fallen-wood hollow**, separate from the Raven's roost. The editor's live MCP endpoint is available on `127.0.0.1:8000`; `/Game/Maps/Island` is loaded.

## What exists now

- The Island has 187 actors (147 `StaticMeshActor`s). There is no Rat home marker or tagged hollow.
- `Roost_East_FallenWood` and `Roost_East_TrunkSupport` are part of the Raven roost, and both render `/Engine/BasicShapes/Cylinder.Cylinder`; they are stretched placeholder cylinders, not a hollow-log asset. Do not reuse them for the Rat.
- `Prop_InnFirewood` is `/Game/Generated/Tripo/FirewoodStack/FirewoodStack/StaticMeshes/FirewoodStack.FirewoodStack`; `Prop_TideglassBench` is the generated DriftwoodBench. Neither is a shelter.
- The PCG forest volume is at `(-80000, -80000, 500)` and spans roughly 250 m square. Live line traces found dense foliage there, but several apparently flat coarse-grid points were small high patches with water at log-scale offsets. No safe Rat anchor was chosen in that forest area.

## Provisional anchor to validate

The existing `NavMeshBoundsVolume_Agent` covers `x=-105900..-95900`, `y=95460..105460`. A quiet candidate within its bounds is `(-105000, 104750)`: a 5 m terrain grid has about 40 cm relief around a 10 m neighborhood; a 25-point 1 m grid measured 15.6 cm total relief over 4 m, with no water hits. The live editor view shows open brown ground with sparse low grass, not concealment. This candidate is roughly 50 m from the Inn and well away from the Raven roost; it needs a small foliage screen and an in-world camera/nav check before it becomes a resident location.

## Asset direction

`Scripts/AssetPipeline/assets/rathollow.py` defines an original 2.6 m weathered trunk, a 0.78 m entrance, a sheltered straw bed, restrained moss patches, and collision strips that leave the passage open. It uses the local ComfyBlender pipeline and shared textures; it does not call Tripo or consume API credits. Blender 5.2.2 exported it successfully: manifest bounds are 256 x 136 x 134 cm, about 1,876 triangles, with four `UBX_` collision strips. The source and FBX are built, but the asset has **not** been imported or visually inspected yet; the standard preview is an EEVEE render, which I left for a time when the editor is closed. Do not assign coordinates to the Rat's lived-memory `places.json` yet.

Next: import with `Scripts/Import-ComfyBlenderAsset.py` when the editor is closed; inspect its material, silhouette, scale, and collision in UE; then decide whether to place it at the provisional anchor. Dress the site lightly, verify floor/entrance clearance and actual navmesh, then wire the Rat's home/spawn. Keep it independent from the Raven's nest.
