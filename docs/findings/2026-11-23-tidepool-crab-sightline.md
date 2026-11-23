# Tideglass crab sightline — 2026-11-23

## Change

Daytime crab shore offsets now have one shared definition for crab spawning and
ground-cover placement. Island scatter reserves a roughly 1.45 m radius around
each of the two existing shore sites, expanded by each plant's horizontal mesh
footprint. The clearance is local to Tideglass and does not reduce general
ground-cover density. The saved map and canonical world state were not changed.

## Verification

- UE 5.8 `CaptiveSky_2Editor` built successfully (4 actions, 27.90 seconds),
  including the final `IslandWeather.cpp` changes.
- A 60-second standalone Game run reached Island and ended normally after 60.3
  seconds with zero model requests. It used an isolated data root and captured
  five 1280×720 frames at the established crab-aimed Tideglass camera.
- Frames 004–005 show the crab's shell clearly within a small open patch.
  Foreground broad leaves still hide parts of its lower legs, so the sightline
  is improved but not complete. See
  `Saved/Playtests/TideglassCrabSightlineFixed_20261123/Screenshots/005_Shorelife__Close.png`.
- The editor and standalone Game were both using the GPU. The Game used a
  temporary 2 GB texture-streaming pool and 720p output; one GPU memory sample
  showed 9,103 MiB of 12,282 MiB in use. This visual check is not a comparable
  performance benchmark.

## Limits and next step

The capture confirms the authored crab is present at its normal daytime shore
site and the shell is easier to distinguish. It does not confirm resident/player
interaction, player-controlled shoreline traversal, or resident feedback. The
crab material still reads lighter than its Blender source preview; review the
imported material under matched lighting before revising the authored mesh.
