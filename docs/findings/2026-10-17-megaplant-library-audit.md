# Megaplant canopy inventory before woodland integration

## Filesystem evidence

The local, ignored Unreal `Content` tree contains four imported tree families:
Common Hazel, European Aspen, European Beech, and Norway Maple. The library
currently contains 296 `.uasset` packages. Each family has A/B/C/D tree
variants; the library also includes PVE presets/data and skeleton packages.
This proves the Fab library is present locally, but filenames alone do not
establish whether the canopy variants are `UStaticMesh`, `USkeletalMesh`, or
PVE-specific assets, nor whether they are suitable for the existing HISM scatter.

The current Island scatter in `AIslandWeather` assigns one spruce mesh to its
HISM and builds deterministic spruce groves. Adding these four families could
give the woodland a more convincing mixed-canopy silhouette without increasing
global ground-cover density, but asset type and render cost must be checked
first.

## Read-only audit

[`Scripts/Audit-MegaplantLibrary.py`](../../Scripts/Audit-MegaplantLibrary.py)
queries the Unreal Asset Registry and reports the class and package path of
tree A/B/C/D variants and `PVE_` assets to the Output Log. It does not load or
modify assets and does not save the Island map.

## Validation and next step

The audit script passes Python syntax compilation. It has not yet been run
inside Unreal because the current desktop connection does not expose the
editor. Once available, run it from the Editor Python console, then select one
confirmed static canopy mesh for a disposable, small-count woodland preview.
Compare its silhouette/materials and frame-time against the existing spruce
view before considering a deterministic species mix. Do not alter the map or
increase the full-island tree budget based only on this filesystem inventory.
