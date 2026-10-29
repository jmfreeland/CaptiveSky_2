# Broadleaf canopy profiles (2026-10-29)

## Change

Runtime broadleaf placement previously scaled maple, aspen, beech, and black
alder to the same 9.5–17 m height range. That gave the four-species generator
less visible vertical structure than its species counts suggest. Each
species now has a bounded, seeded height profile while retaining overlap:

| Species | Height range |
| --- | ---: |
| Norway maple | 10–15 m |
| European aspen | 12.5–18 m |
| European beech | 10.5–16 m |
| Black alder | 9–14 m |

The ranges add a taller aspen layer and lower moisture-edge alder layer without
changing tree placement, per-species quotas, the 3,600 broadleaf-instance
budget, collision, or cull settings. No new assets or runtime instances were
added. The seeded random stream supplies the within-species height sample.

## Validation and limits

- UE 5.8 `CaptiveSky_2Editor` compiled and linked successfully in this
  worktree after adding pure automation coverage for profile endpoints,
  midpoint, clamping, and invalid inputs.
- A follow-up headless NullRHI automation launch crashed before engine init at
  `SecondsSinceStart=0`, with the same access-violation call-stack hash
  `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D` recorded in the prior isolated
  startup attempt. The new automation tests therefore compiled but did not
  execute. Do not repeat this identical isolated launch without investigating
  the startup path.
- The main project editor is running a different checkout; it has not loaded
  this source change. No PIE, asset save, or resident-model request was used
  for this milestone. Species silhouettes and visual naturalness remain to be
  checked in the main project at a daylight viewpoint when GPU headroom allows.

## Next

Integrate the source change into the project editor's checkout, then make a
short daylight capture with resident thinking disabled. Compare maple/aspen/
beech/alder silhouettes and record FPS and whole-device VRAM separately from
the existing texture-pool measurements. Preserve the fixed tree budget and
30-FPS target while judging the result.

The live project also has the `ProceduralVegetationEditor` plugin enabled and
contains a `ProceduralVegetation` asset plus skeletal-mesh variants for
Aleppo Pine under `/Game/Megaplant_Library/Tree_Aleppo_Pine/`. This is a
potential new evergreen silhouette, but it is a Mediterranean species and
should not be scattered across the temperate Island indiscriminately. Before
integration, inspect the PVE preset itself and establish a lower-cost far
representation; if it remains desirable, consider a small, clearly introduced
sheltered grove within the existing tree-count budget.

## Live-project foliage preflight follow-up

Read-only Unreal asset queries found that each of the four currently used
Megaplant skeletal trees has exactly one LOD: Norway Maple 4,203 vertices,
European Aspen 6,231, European Beech 6,005, and Black Alder 11,781. Aleppo
Pine A also has one LOD and 9,189 vertices. Its two material slots depend on
the Procedural Vegetation sample foliage master; its four referenced species
textures are Bark Color 4096², Bark NAH 4096², Foliage Opacity 2048², and
Foliage Normal 4096². The mesh depends on `DynamicWind` and a PVE asset, so it
is not just a low-cost static-tree substitute. These dimensions are source
texture sizes, not measured resident VRAM.

This rules out adding Aleppo Pine as a broad scatter under the current memory
uncertainty. The stronger next step is a repeatable foliage LOD/texture-budget
path for the already-used tree family; then compare a low-cost far representation
before considering any additional species. No asset was edited and no runtime
memory claim is made from registry metadata.

The PVE preset is a `ProceduralVegetationGrowerPreset` with a separate
`ProceduralVegetationGrowthDataAsset`. Its reflected settings are 40 growth
cycles, a 16 m trunk target, three branch generations, and foliage density 3.
The enabled `ProceduralVegetationEditor` plugin describes its generated
vegetation as Nanite-Foliage-ready, while its preset schema says simulation cost
grows roughly linearly with growth-cycle count. This is a promising editor-time
authoring route for making a lighter, lower-cycle variant with a different
silhouette; it is not evidence that the current Aleppo skeletal output has
Nanite, additional LODs, or lower runtime memory. Next, generate one isolated
variant only after the open GPU has cooled, inspect its output class/LOD/texture
dependencies, and keep it outside the Island population until that preflight
passes.

## PVE export trade-off

The installed UE 5.8 `ProceduralVegetation` export settings expose both
`StaticMesh` and `SkeletalMesh` output. `bCreateNaniteFoliage` defaults to true.
The plugin's setting description explicitly states that static output has no
skeletal wind animation, while skeletal output includes bones and can receive a
Dynamic Wind preset. This offers a plausible two-distance strategy: retain a
wind-responsive skeletal form near the player and test a Nanite static form for
far silhouettes. It is not yet verified that the Aleppo graph can export both
representations from the same generated data or that the transition would read
well. The next PVE experiment should use an isolated copy, preserve the source
asset, test `Append` rather than `Replace`, and compare geometry, wind response,
texture dependencies, and in-game frame/VRAM cost before any Island placement.
