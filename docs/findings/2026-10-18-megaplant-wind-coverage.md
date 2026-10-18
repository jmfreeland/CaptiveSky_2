# Megaplant DynamicWind profile coverage (2026-10-18)

## Result

The isolated `CaptiveSky2.Agent.InstancedSkinnedFoliageProvider` test now checks
every installed PVE tree source mesh in the local library: Common Hazel,
European Aspen, European Beech, and Norway Maple. For each present asset it
requires the `UDynamicWindSkeletalData` profile to exist and be enabled, then
checks that `AIslandInstancedSkinnedFoliage` selects `UDynamicWindData`, assigns
it to the component, and clears the generated provider when its source mesh is
removed. Missing pack assets are reported as informational skips so source-only
clones can still run the generic provider lifecycle checks.

The UE 5.8.3 editor target built successfully in the isolated
`Codex_VisitorResonance_20261017` scratch project, whose `Source` is a junction
to the current repository and whose `Binaries` are separate from the user's
main editor. The scratch `.uproject` was aligned with the main project's
`DynamicWind` plugin setting. The NullRHI automation passed with all four local
assets present; the run log is
[`Codex_MegaplantWindSpecies_20261018.log`](../../Saved/Logs/Codex_MegaplantWindSpecies_20261018.log).
No asset or Island map was saved, and no model request was made.

## Limits and next validation

This proves the imported tree meshes carry enabled wind-profile data and that
the batched actor binds the expected UE transform provider. NullRHI does not
prove visible deformation, wind response magnitude, in-world scale, visual
quality, or runtime cost. The test does not place any Megaplant on the Island.

The earlier Beech asset-build log,
[`Codex_MegaplantWindProvider_20261018.log`](../../Saved/Logs/Codex_MegaplantWindProvider_20261018.log),
reported that voxel shape preservation and Nanite assemblies were disabled by
project settings. Keep those settings unchanged until a separate matched
real-RHI foliage profile establishes whether enabling them helps this project.
The next meaningful integration gate is a disposable, un-saved Island preview
using a small number of the four species, followed by matched frame-time and
texture-pool measurements before proposing any saved-map placement.
