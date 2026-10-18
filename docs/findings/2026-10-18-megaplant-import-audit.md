# Megaplant import audit and Unreal review gate

## Current local evidence

Read-only inventory of the working project on 2026-10-18 found four imported
species under `Content/Megaplant_Library/`:

| Species | `.uasset` packages | Approx. package size | Baked variants |
|---|---:|---:|---|
| Common hazel | 63 | 149.5 MiB | A, B, C, D |
| European aspen | 78 | 139.6 MiB | A, B, C, D |
| European beech | 73 | 184.1 MiB | A, B, C, D |
| Norway maple | 82 | 156.7 MiB | A, B, C, D |

Each species folder contains PVE generation data and a preset as well as its
four named tree variants. The project's `CaptiveSky_2.uproject` enables
`ProceduralVegetationEditor`; the matching UE 5.8 engine plugin descriptor is
present under `Engine/Plugins/Experimental`, version 1.0 and marked
experimental. Its descriptor declares editor and runtime modules and depends
on Dataflow, GeometryScripting, PCG, and DynamicWind.

The existing `Content/Plants/Meshes/` inventory remains `Festuca_gautieri_LD`,
`Phalaris_arundinacea_LD`, `Rhododendron__Everestianum__HD`, and
`Typha_latifolia_LD`. `Content/PCG/PCG_World_Forest` is also present. This
corrects the older inventory statement in the 2026-10-10 visual-priority note
that Megaplant assets were not yet in the project tree; the current local tree
has since changed.

## What this does not prove

Package presence and the `.uproject` plugin flag do not prove that the current
editor loaded PVE successfully, that a preset can generate a mesh, or that the
result renders, shades, deforms in wind, streams textures, or performs well.
No asset package was opened and no Unreal process, Content package, PCG graph,
map, or saved world state was changed for this audit. The current editor is
nonresponsive and has been left running.

## Next validation gate

When the editor is responsive, inspect one Hazel and one Aspen preset and
their A-D variants in an isolated disposable level. Record actual bounds,
Nanite/LOD and shadow settings, material/texture dimensions, PVE generation
success, and whether the imported tree responds to the existing weather wind
system. Then try a small, reversible mid-distance cluster in a scratch copy of
the Island, preserving landmark sightlines and comparing against a matched
11:00 baseline. Keep the 30-FPS p95 target and texture budget. Do not remove
the existing USD plant versions or replace any saved-Island placement until a
real-RHI review confirms the Megaplant variant is a better, safe replacement.
