# Landscape shoreline layer: handoff for visual verification (2026-10-10)

Built by Claude, **not yet verified visually**. The user asked for this to be handed to Codex to test, because a capture run loads
the full Island map in a second editor and the machine was too tight on memory to do that beside the user's open editor
(10 GB editor, commit at 83%, ~9 GB free).

## What exists

New assets only. The authored `/Game/Materials/MI_Island_Landscape` still uses `M_Island_Textured_Wet` and was not touched.

| Asset | What it is |
|---|---|
| `/Game/Generated/ComfyBlender/Library/T_Lib_ShoreSand_{BC,N,ORM}` | the one new texture set: tileable beach sand made by the ComfyUI asset pipeline (SDXL, seed 620, prompt in `Scripts/AssetPipeline/workflows/lib_shoresand.json`), PBR maps from `pbr.py`. 1024², wraps. |
| `/Game/Materials/M_Island_Textured_Shore` + `MI_Island_Landscape_Shore` | copy of the wet graph with a **sand stage inserted before the wet stage**, so rain still darkens/glosses the sand. The instance is a copy of `MI_Island_Landscape_Wet` (all authored layer overrides kept). |
| `/Game/Materials/M_Island_Textured_ShoreSubstrate` + `MI_Island_Landscape_ShoreSubstrate` | the same plus a Substrate water film (below). **Experimental.** |

Scripts (commit `9f07d39`): `Scripts/Create-LandscapeShoreMaterial.py`, `Scripts/Create-LandscapeShoreSubstrate.py`. Both rebuild their own assets
every run. Because the shore parent is a snapshot of the wet parent, **rerun `Create-LandscapeShoreMaterial.py` after any rebuild of
`Create-LandscapeWetMaterial.py`**. Run them in a headless scratch editor with a path-without-spaces stub (see the docstrings; `Saved/CompileScratch/Claude_Props`
was used, with `-ExecutePythonScript`, `-RenderOffscreen`, `-NoZen`, local DDC and shader working dir, and a `QUIT_EDITOR` at the end of the stub).

### Why a shoreline layer

The audit parameter dump (`Saved/Logs/Codex_LandscapeAudit_Automation_20260930.log`) shows the auto-material's layers are Rocky Ground, Mossy Grass, Rocky
Ground (mid-high), Rock Cliff, Windswept Snow and Asphalt. There is nothing for the shore, so the beach and the sea floor in the shallows (which the Water plugin shows through)
are all the ground texture. Anti-tiling is already in the graph (`Random Tiling Levels/Scale`, texture and colour variation), so that is not a gap.

### Sand stage (group `Shore`, all tunable on the instance without a rebuild)

| Parameter | Default | Meaning |
|---|---|---|
| `SeaLevelCm` | 940 | ocean Z (`WaterBodyOcean` at z=940, `Scripts/Create-IslandWaterBody.py`) |
| `ShoreTopCm` | 170 | sand reaches this far above sea level (and covers everything below it) |
| `ShoreFadeCm` | 90 | soft top edge |
| `ShoreBreakupCm`, `ShoreBreakupTileCm` | 60, 900 | noise (`T_LandscapeNoise`) wobble of the waterline |
| `ShoreSlopeMin`, `ShoreSlopeFade` | 0.78, 0.12 | sand only where vertex-normal Z is above this (about under 37 degrees), so cliffs keep their layers |
| `SandTileCm`, `SandMacroTileCm`, `SandMacroMix` | 350, 1700, 0.4 | two-scale sampling to hide repetition |
| `SandNormalIntensity` | 1.0 | |
| `SandWetBandCm`, `SandDampDarken` | 45, 0.55 | permanent damp, darker, glossier sand right at the waterline |
| texture params | | `Shore Sand Base Color`, `Shore Sand Normal`, `Shore Sand ORM` |

The sea level and band numbers are **derived, not measured**: the island's shore profile was never sampled (loading the map was the memory problem). If the sand
band looks too high, too low or too thin, tune `ShoreTopCm` / `ShoreFadeCm` first.

### Substrate water film (group `Shore Film`)

The project already runs Substrate (`r.Substrate=True`, 4 closures per pixel) but the landscape graph is legacy material attributes that Substrate auto-converts, so wetness is
faked with darkening and roughness. 5.8.3 exposes `Substrate Convert Material Attributes`, `Substrate Vertical Layer`, `Substrate Coverage Weight` and the Slab BSDF.
The variant converts the finished legacy attributes, layers a clear dielectric slab on top (DiffuseAlbedo 0, F0 0.02, F90 1, roughness `FilmRoughness` 0.03; normal defaults to
the vertex normal, so the film is flat) weighted by coverage, and connects the result to the **Front Material** root pin (the legacy attributes stay connected too).
Coverage = max(rain film, waterline film): rain film is the wet stage's puddle noise pattern (`FilmTileCm` 1200, `FilmCoverage` 0.36, `FilmSharpness` 6.5, flat ground only) times
`Wetness` from `MPC_IslandEnvironment`, plus `FilmSheen` 0.25 everywhere flat when wet; waterline film is sand within `FilmShoreBandCm` 60 of `FilmSeaLevelCm`
at `FilmShoreAmount` 0.7. Note the puddle numbers duplicate the wet stage's; they are not linked.
Pin gotcha found while building: the Vertical Layer's `Base` input is named **`Bottom`** for `connect_material_expressions`.

## Verification results (2026-10-10)

Codex ran the captures on the main project (`Saved/Logs/Codex_Shore{Baseline,Legacy,Substrate}_20261010.log`, frames in
`Saved/Viewpoints/2026-10-10_{135837,140148,140439}_h12.0/01_ShoreApproach.png`, hour 12); Claude read the frames. All three runs passed `CaptiveSky2.Visual.Viewpoints`.

- **Legacy shore variant renders and the sand is where it should be.** A beach band appears along the foreground waterline with an organic, noisy upper edge, meeting the
  Water plugin's turquoise shallows; the rowboat sits on it. No checkerboard, black, magenta or errors. The ground away from the beach is not changed by the material
  (the sand mask is zero there); the pixel differences elsewhere are the sky and the **cloud shadows on the ground**, because each capture is a separate process with a different
  dynamic cloud state. So the baseline-vs-shore frames are not a controlled pixel comparison, and there is no same-session baseline-vs-parent pair in the harness yet.
- **The Substrate variant also renders.** The landscape pipeline accepts a Substrate Front Material: no fallback, no error. It differs from the legacy variant in the right place:
  a darker, glossier damp-sand band along the waterline instead of pale sand fading straight into the shallows (beach-region mean difference 9.4/255 vs 5.7 on plain ground).
  The film's rain/puddle part was **not** exercised (dry frame); pair it with `-CompareLandscapeWetness` / `-LandscapeWetness 1` to see it.
- **The sand is bright.** In the legacy frame the beach reads about (218, 203, 177) mean RGB against about (98, 94, 87) for the surrounding ground, about 2.2 times brighter (not clipped:
  0% of sand pixels at 250 or more). The builder script now has a `SandAlbedoScale` parameter (default 0.8) for this; **the assets have not been rebuilt with it yet**, because a rebuild
  deletes and recreates the materials a capture may be using. Until then, tune `SandDampDarken` or rebuild. The sand grain is not visible at this camera distance.
- **Not yet checked:** other viewpoints (`00_Survey`, `02a_TideglassGroundDetail`, `04_WindArchOverlook`), hour 17 / low sun, wet versus dry, frame time (the Substrate variant adds a closure per pixel on the
  landscape), whether the sand band suits the rest of the shore (the band is derived from sea level Z=940, not measured), and in-game (PIE) behaviour. The authored `MI_Island_Landscape` is unchanged.

## Pitfalls hit while verifying

- Git Bash rewrites an argument that starts with `/` (`-ViewpointLandscapeParent=/Game/...` became `C:/Program Files/Git/Game/...`), so the test reported "Landscape preview parent material loaded" is null. Use PowerShell,
  or `MSYS_NO_PATHCONV=1`. The package path (`/Game/Materials/M_Island_Textured_Shore`) is fine.
- Starting a second editor beside the user's editor exhausted memory (commit 98%). Separately, running ComfyUI on the same GPU while the editor held 8.3 of its 9.2 GB VRAM budget coincided with an editor
  `D3D12` GPU page-fault crash (dump `Saved/Logs/D3D12.0.2026.10.10-13.31.37.nv-gpudmp`). Do not run ComfyUI or Blender GPU bakes beside an open editor.

## How to test (no authored asset needs to change)

The capture harness swaps the landscape's parent in memory and restores it. Close the editor on this project first (the script says so; two editors exhausted memory).
From the repo root, same hour, same viewpoint, three runs per viewpoint:

```powershell
$v = "01_ShoreApproach"   # also: 00_Survey, 02a_TideglassGroundDetail, 04_WindArchOverlook
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only $v -NoWorldState -LogPath Saved/Logs/ShoreBaseline.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only $v -NoWorldState -LandscapeParent /Game/Materials/M_Island_Textured_Shore -LogPath Saved/Logs/ShoreLegacy.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only $v -NoWorldState -LandscapeParent /Game/Materials/M_Island_Textured_ShoreSubstrate -LogPath Saved/Logs/ShoreSubstrate.log
```

Repeat at `-Hour 17` (low sun is where a water film should show most), and for wet-vs-dry with `-CompareLandscapeWetness` plus `-LandscapeParent <each>` at `01_ShoreApproach` and
`02a_TideglassGroundDetail`. Prior notes say to use a scratch project with a junctioned `Content` and `-NoHotReloadFromIDE`-style builds if the main editor must stay open.

What a pass looks like:
- the shoreline reads as pale sand with an organic, noisy upper edge, darker and glossier right at the water; no sand on steep rock or cliff;
- no tiling pattern visible across the beach (two-scale sampling) and no obvious seam at the sand/ground blend;
- unchanged look away from the shore (compare mean absolute RGB difference of a frame region far from the water against the baseline: it should be near zero);
- no new errors in the log; frame time not meaningfully worse (the Substrate variant adds a second closure per pixel on the landscape: check a frame-time number at `02a`).
If the Substrate variant fails, discard `M_Island_Textured_ShoreSubstrate` and keep the legacy shore variant.

## Authoring (needs the user's approval; not done)

`LANDSCAPE_SHORE_AUTHOR=1` with `Create-LandscapeShoreMaterial.py` reparents `MI_Island_Landscape` onto the shore graph after copying it once to
`MI_Island_Landscape_PreShoreBackup`; `=0` restores the wet parent. The authored instance currently uses the wet parent.

## What was checked about 5.8 so it need not be redone

- Official [5.8 release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes?lang=en-US): the terrain item is **Mesh Terrain (Experimental)**, a wholesale replacement
  for heightfield landscapes (not appropriate here). Lumen Lite is Beta; MegaLights is Production Ready in 5.8 but `r.MegaLights.EnableForProject=False` in this project (a lighting setting, left as a separate decision).
  The notes list landscape grass-type and grass-weight-shader changes, which matter only if the island moves from runtime HISM ground cover to Landscape Grass.
- A [forum report](https://forums.unrealengine.com/t/nanite-displacement-bugged-in-exact-same-landscape-in-ue-5-8-but-not-in-ue-5-7/2739815) says Nanite displacement on a landscape auto-material regressed in 5.8,
  so it was not used.
- The engine's own headers (`Engine/Source/Runtime/Engine/Public/Materials/MaterialExpressionSubstrate.h`, `Material.h`) confirm the Substrate nodes and the `FrontMaterial` root input exist in 5.8.3.
  No stochastic-sampling option exists on the texture-sample header, and the graph already has its own anti-tiling.
