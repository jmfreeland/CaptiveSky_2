# Rat hollow: live Island audit

The requested home is a **hidden fallen-wood hollow**, separate from the Raven's roost. The editor's live MCP endpoint is available on `127.0.0.1:8000`; `/Game/Maps/Island` is loaded.

## What exists now

- The Island has 187 actors (147 `StaticMeshActor`s). There is no Rat home marker or tagged hollow.
- `Roost_East_FallenWood` and `Roost_East_TrunkSupport` are part of the Raven roost, and both render `/Engine/BasicShapes/Cylinder.Cylinder`; they are stretched placeholder cylinders, not a hollow-log asset. Do not reuse them for the Rat.
- `Prop_InnFirewood` is `/Game/Generated/Tripo/FirewoodStack/FirewoodStack/StaticMeshes/FirewoodStack.FirewoodStack`; `Prop_TideglassBench` is the generated DriftwoodBench. Neither is a shelter.
- The PCG forest volume is at `(-80000, -80000, 500)` and spans roughly 250 m square. Live line traces found dense foliage there, but several apparently flat coarse-grid points were small high patches with water at log-scale offsets. No safe Rat anchor was chosen in that forest area.

## Provisional anchor to validate

The existing `NavMeshBoundsVolume_Agent` covers `x=-105900..-95900`, `y=95460..105460`. A quiet candidate within its bounds is `(-105000, 104750)`: a 5 m terrain grid has about 40 cm relief around a 10 m neighborhood; a 25-point 1 m grid measured 15.6 cm total relief over 4 m, with no water hits. The live editor view shows open brown ground with sparse low grass, not concealment. This candidate is roughly 50 m from the Inn and well away from the Raven roost; it needs a small foliage screen and an in-world camera/nav check before it becomes a resident location.

**Live recheck (2026-10-10):** an Unreal MCP viewport capture aimed at this point from `(-106000, 103250, 3200)` again shows broad exposed brown ground, sparse low cover, and no nearby canopy. A center trace from Z=20,000 to -2,000 cm returned 17,669.5 cm, putting the hit at approximately Z=2,330.5 cm; this is consistent with the existing terrain survey, not a validation of shelter. PVE reports enabled in the connected editor. No actors were placed, and the map remains untouched. Do not assign this point as the Rat's home unless a temporary foliage-screen preview makes it read as genuinely hidden and confirms that the entrance/approach remain navigable.

## Asset direction

`Scripts/AssetPipeline/assets/rathollow.py` defines an original 2.6 m weathered trunk, a 0.78 m entrance, a sheltered straw bed, restrained moss patches, and collision strips that leave the passage open. It uses the local ComfyBlender pipeline and shared textures; it does not call Tripo or consume API credits. Blender 5.2.2 exported it successfully: manifest bounds are 256 x 136 x 134 cm, about 1,876 triangles, with four `UBX_` collision strips. On 2026-10-10 the live editor imported it to `/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01` without changing the Island map. `unreal_import.json` reports 256.4 x 135.7 x 134.1 cm, all four material slots assigned, four box collision shapes, and no validation errors. The editor's static-mesh preview confirms the weathered wood surface and expected 1,876 triangles / two UV channels. Looking at the entrance end-on revealed that the wooden inner cap occludes the cavity and the intended dark backing; I removed that inner cap from the source. The already-imported Unreal asset is still the pre-fix version until it is regenerated and reimported after the editor closes. Do not assign coordinates to the Rat's lived-memory `places.json` yet.

The editor was running the `captivesky_2_claude` worktree, whose `Saved/` directory does not contain this generated source pack, while its `Content/` is junctioned to the main checkout. The importer now accepts an explicit `COMFYBLENDER_SOURCE_ROOT` for this legitimate multi-checkout case; import output still goes to the currently open project's `/Game/Generated/ComfyBlender/`.

Next: after closing the editor, rebuild and reimport RatHollow; inspect that the cavity and nest lining read clearly and that the passage remains unobstructed. Then place a temporary copy near—but not at—the provisional anchor for scale, concealment, floor/entrance clearance, and actual navmesh checks. Only after those pass should the Rat receive a home/spawn anchor. Keep it independent from the Raven's nest.
