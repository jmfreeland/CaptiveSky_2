# Megaplant woodland regeneration candidate — rejected, 2026-10-17

## Decision

Do not ship the proposed HISM regeneration layer. The four selected `_C`
packages exist, but inspection of their serialized class names and the project's
asset audit identifies them as `SkeletalMesh`, not `UStaticMesh`. The original
`FObjectFinder<UStaticMesh>` implementation would fail to load them, leaving the
scatter empty; its automation test would correctly fail on typed asset loading.
That implementation and test have been withdrawn from the source tree.
The IslandWeather and ecology-test files are back to their pre-candidate state;
no Content package or map was written.

This matches the earlier UE 5.8 instanced-skinned probe: the component can load
these assets, but foliage is sparse/bare with the normal renderer settings,
becomes dark and coarse when Nanite foliage is enabled, and has no transform
provider or bounded-batch cost evidence. Do not use the installed skeletal
outputs as Island canopy or enable experimental Nanite foliage project-wide.

## Evidence

- UE 5.8's existing `Preview-InstancedSkinnedMegaplant.py` report confirms the
  candidates load as skeletal meshes; the earlier probe records material slots,
  bounds, transient tint behavior and the limitations above.
- A later editor-target build compiled the proposed `IslandWeather.cpp` and
  `IslandEcologyTests.cpp`, but this only proves C++ compilation, not that the
  typed content references resolve. The final module link could not replace
  `Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` because the running editor holds
  it; the proposed automation was never run.
- `git diff --check` passes for the current source changes.

## Next step

When the editor can be safely closed, use PVE's editor export UI to create one
complete static-mesh candidate in a disposable test folder. Preview it in the
fixed Tideglass-to-Woodland Game composition, check foliage-material usage and
silhouette, then measure a small instanced batch before any Island placement.
Do not increase global density as a substitute. See
[`2026-10-13-instanced-skinned-megaplant.md`](2026-10-13-instanced-skinned-megaplant.md)
for the prior component/renderer probe.
