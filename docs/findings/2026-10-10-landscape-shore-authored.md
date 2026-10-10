# Shoreline sand + Substrate water film authored onto the Island landscape (2026-10-10)

`/Game/Materials/MI_Island_Landscape` (the instance on all 4,096 landscape components) now uses **`/Game/Materials/M_Island_Textured_ShoreSubstrate`**: the wet landscape graph, plus a shoreline sand stage
built from one ComfyUI-pipeline texture (`T_Lib_ShoreSand_*`), plus a Substrate water film. Background and parameters:
[`2026-10-10-landscape-shore-handoff.md`](2026-10-10-landscape-shore-handoff.md) (Codex's verification notes live there).

## What shipped, and which 5.8 feature it uses

- **New pipeline texture:** `T_Lib_ShoreSand_{BC,N,ORM}` (SDXL beach sand, three prompts to get fine grain without pebbles or flat beige), a shoreline layer the auto-material never had. Sand from the sea floor up to `ShoreTopCm` above sea level (Z=940),
  gentle slopes only, noisy edge, permanent damp band at the waterline, `SandAlbedoScale` 0.8 (the first capture had the sand about 2.2x brighter than the ground).
- **Substrate (on in this project, `r.Substrate=True`, but unused by the landscape until now):** the finished legacy attributes go through `Substrate Convert Material Attributes`, and a clear dielectric water slab
  (F0 0.02, F90 1, low roughness, flat normal) is layered on top with `Substrate Vertical Layer`, weighted by a `Substrate Coverage Weight` (puddles, a rain sheen, and a permanent film on the waterline sand), connected to the **Front Material** root pin.
  The layer uses **parameter blending** (merge the film and the ground into one BSDF instead of evaluating both), which is Substrate's cost lever for a surface that covers the screen.
- Not used, on purpose: Mesh Terrain (Experimental, a wholesale replacement for heightfield landscapes), Nanite displacement on landscapes (a [forum report](https://forums.unrealengine.com/t/nanite-displacement-bugged-in-exact-same-landscape-in-ue-5-8-but-not-in-ue-5-7/2739815) says it regressed in 5.8),
  stochastic sampling (no such option on the texture-sample node; the graph already has its own anti-tiling). Official [5.8 release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes?lang=en-US).

## Evidence (real-RHI, UE 5.8.3, main project, no editor open)

| Check | Result |
|---|---|
| Shipped state (no override), shore view 12:00 and 17:00 | pass; golden-tan beach with an organic edge at the waterline; at 17:00 the sand reads warm, with a darker damp band at the water |
| Same-session dry/wet pairs on the shipped state, shore view 17:00 | pass; wet ground reflects the sky (mean RGB about 86, 90, 98), the beach darkens and glosses (160, 115, 70 dry to 108, 81, 70 wet) |
| Ground-detail view, dry | identical with and without the sand stage (mean abs diff 0.06/255), so ordinary ground is unchanged |
| Ground-detail view, wet | pooled water with sky reflection; the Substrate pools are deeper than the legacy roughness-only pools |
| Parameter blending vs full layering, wet shore frame | visually indistinguishable (same reflection, same wet sand) |
| Night (22:00), shore view | pass; the beach reads as a pale moonlit strip against near-black ground, turquoise shallows beside it |
| `CaptiveSky2.Agent.IslandLandscapeAssignment`, `CaptiveSky2.Agent.IslandEnvironment` | pass; assignment log: `parent=/Game/Materials/M_Island_Textured_ShoreSubstrate` |

## Cost: what was and was not measured

Pixel-shader instruction counts (`MaterialEditingLibrary.get_statistics`; base pass only, so it **does not capture Substrate's lighting-pass cost**, which parameter blending targets):

| Material | PS instructions | vs original auto graph |
|---|---|---|
| `M_Island_Textured_Auto` | 263 | |
| `M_Island_Textured_Wet` (previously authored) | 289 | +10% |
| `M_Island_Textured_Shore` (legacy variant, kept as fallback) | 348 | +32% |
| `M_Island_Textured_ShoreSubstrate` (authored) | 492 | +87% |

Capture throughput in the viewpoint harness (`02a_TideglassGroundDetail`, 12:00, `-ViewpointGroundCover`, three interleaved rounds, parent swapped each run): raw FPS rounds wet 25.7 / 31.7 / 26.4, shore 25.4 / 34.5 / 27.3,
Substrate 31.4 / 27.3 / 25.3; medians 26.4, 27.3, 27.3. **No measurable difference**: the spread within one material (about 25 to 34) is larger than the difference between them. The harness is dominated by foliage render-thread cost
(the project's own profile, `docs/findings/2026-10-04-actual-gameplay-profile.md`, has GPU about 18 ms inside a 79 ms render-thread-bound frame), so it **cannot isolate the landscape's GPU cost**. The decision to ship Substrate rests on that headroom,
the unchanged look with parameter blending, and reversibility, not on a measured GPU saving. If the foliage CPU cost is later fixed and the frame becomes GPU-bound, recheck this first.

## Revert / switch

- Back to the legacy shore graph (cheaper, no Substrate): `LANDSCAPE_SHORE_AUTHOR=0 Scripts/Create-LandscapeShoreSubstrate.py`.
- Back to the previous wet graph: then `LANDSCAPE_SHORE_AUTHOR=0 Scripts/Create-LandscapeShoreMaterial.py`, or set the instance's parent to `M_Island_Textured_Wet`, or restore **`MI_Island_Landscape_PreShoreBackup`** (a copy of the instance as it was).
- Rebuild order after any rebuild of `Create-LandscapeWetMaterial.py`: `Create-LandscapeShoreMaterial.py`, then `Create-LandscapeShoreSubstrate.py` (each is a snapshot of the one before; `LANDSCAPE_SHORE_AUTHOR=1` on the Substrate script reauthors it).

## Still not checked

In-game / PIE behaviour (a real rain transition, the Water plugin's shallows in motion), a GPU-time (not throughput) measurement, other viewpoints beyond the shore and ground-detail views, and low-end hardware. The sand band comes from sea level, not a measured shore
profile; `ShoreTopCm` / `ShoreFadeCm` on the instance are the first knobs. Captures must run on the main project: the scratch project's `IslandViewpoints.json` is older. Git Bash mangles `/Game/...` arguments (use PowerShell or `MSYS_NO_PATHCONV=1`).
