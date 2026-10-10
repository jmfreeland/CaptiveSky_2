# Shoreline sand layer authored onto the Island landscape (2026-10-10)

`/Game/Materials/MI_Island_Landscape` (the instance on all 4,096 landscape components) now uses `/Game/Materials/M_Island_Textured_Shore`: the wet graph plus a shoreline sand stage
built from one ComfyUI-pipeline texture (`T_Lib_ShoreSand_*`). Background and parameters are in
[`2026-10-10-landscape-shore-handoff.md`](2026-10-10-landscape-shore-handoff.md) (held by Codex while it records its own verification); this note records what was done afterwards.

## What changed

- Rebuilt `M_Island_Textured_Shore` with `SandAlbedoScale` 0.8 (the first capture had the sand about 2.2 times brighter than the ground), then ran
  `LANDSCAPE_SHORE_AUTHOR=1 Create-LandscapeShoreMaterial.py`: it copied `MI_Island_Landscape` to **`MI_Island_Landscape_PreShoreBackup`** and reparented the authored instance.
- **Revert:** `LANDSCAPE_SHORE_AUTHOR=0` with the same script puts the wet parent back, or set the instance's parent to `M_Island_Textured_Wet` in the editor, or restore the backup.
- **Not authored:** the Substrate water-film variant (`M_Island_Textured_ShoreSubstrate` / `MI_Island_Landscape_ShoreSubstrate`) stays as an opt-in (below).

## Evidence (all real-RHI, UE 5.8.3, main project, no editor open)

| Check | Result |
|---|---|
| Shipped state, no override, `01_ShoreApproach` at 12:00 and 17:00 | passes; the beach band is present at the waterline (`Saved/Viewpoints/2026-10-10_143321_h17.0`, `..._143446_h12.0`) |
| Same-session dry/wet pairs, shore viewpoint, 17:00, both variants | pass; dry sand reads golden-tan (mean about 131, 92, 55) instead of a white stripe; wet darkens ground and sand with no artifacts (`..._142129_h17.0_*`, `..._142325_h17.0_*`) |
| Ground-detail view (`02a_TideglassGroundDetail`), dry | **identical** between the two variants (mean abs diff 0.06/255): ordinary ground is unchanged by the sand stage |
| Ground-detail view, wet | both pool water; the Substrate film's pools are deeper and more reflective (`..._142531_h12.0_*`, `..._142728_h12.0_*`) |
| `CaptiveSky2.Agent.IslandLandscapeAssignment`, `CaptiveSky2.Agent.IslandEnvironment` | both pass; the assignment log reports `parent=/Game/Materials/M_Island_Textured_Shore` |

## Cost, and why the Substrate variant was not authored

Pixel-shader instruction counts from `MaterialEditingLibrary.get_statistics` (a relative proxy; not a frame-time measurement):

| Material | PS instructions | vs original auto graph |
|---|---|---|
| `M_Island_Textured_Auto` | 263 | |
| `M_Island_Textured_Wet` (previously authored) | 289 | +10% |
| `M_Island_Textured_Shore` (now authored) | 348 | +32% (+20% over the wet graph) |
| `M_Island_Textured_ShoreSubstrate` | 492 | +87% (+70% over the wet graph) |

The Substrate water film gives the best wet ground (real sky reflection, deeper puddles) and is accepted by the landscape pipeline, but it nearly doubles landscape pixel cost, and the landscape covers most of
the screen on a project that is already render-bound (`docs/findings/2026-10-04-actual-gameplay-profile.md`). To try it, set `MI_Island_Landscape`'s parent (or a copy's) to `M_Island_Textured_ShoreSubstrate` and compare frame time in PIE.

## Not checked

PIE/in-game behaviour (a real rain transition, the Water plugin's shallows in motion), frame time (only the instruction-count proxy above), viewpoints other than the two used, and night. The sand band is derived from sea level (Z=940),
not measured from the shore profile; `ShoreTopCm` / `ShoreFadeCm` on the instance are the first things to tune. A scratch-project `IslandViewpoints.json` is older than the main one, so run captures on the main project.
