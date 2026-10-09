# The Raven's nest has a woven floor (2026-10-09)

The persistent nest's earlier placeholder weave consisted only of seven short
twigs per layer around the rim. A five-twig, two-course lattice now sits beneath
the first ring, so the silhouette reads more like a shallow bowl than a hollow
stack of hoops. Its slight offsets and angles are seeded from the nest site, so
rebuilding or restoring the same nest keeps the same appearance.

The base is part of the existing visual-only instanced mesh. It adds no
collision, navigation, shelter, or safety semantics, and it changes no saved
world-state fields. A storm that removes the lowest layer also removes the base;
the next weave restores it naturally with the remaining nest.

`CaptiveSky2.Agent.IslandNest` now checks the base's visible instance count,
low/flat placement, deterministic reconstruction, and the existing alternating
rim courses with the added base offset. The isolated UE 5.8.3 editor target
built successfully (158 actions, including UHT and module link) with
`-NoHotReloadFromIDE`, keeping the open editor's binaries untouched. Then
`CaptiveSky2.Agent.IslandNest` passed in that scratch editor with NullRHI,
Python and agent thinking disabled, zero model requests, a 60-second realtime
cap, and scratch-only world data:
[`Codex_IslandNest_20261009.log`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_IslandNest_20261009.log).
The log contains temporary-world teardown warnings, but the test completed
successfully with exit code 0 and no failed assertions. A visual capture is
still needed to judge the placeholder geometry in the live Island.

## Isolated visual-preview attempt (not visually verified)

To keep the user's open editor, unsaved materials, and live world state
untouched, a three-layer `Roost_West` nest preview was placed in the scratch
project's `WorldData/WorldState/Island.json` at the recorded perch point
`(-100640, 101250, 3079.11)`. This fixture is isolated from the main
`WorldState` directory.

The first offscreen capture attempt exited before map load because the installed
Derived Data Cache graph had no writable node; the log explicitly recommends
`-DDC-ForceMemoryCache`. Retrying with that option and scratch-local cache and
shader-working paths got through DDC initialization, but then spent over a
minute compiling cold PCD3D_SM6 editor shaders. The test never reached map or
viewpoint output. The scratch editor used about 4.3 GiB and its shader workers
about 3.4 GiB while the existing editor remained open; the run was interrupted
through its own command session, and its process tree exited. No screenshot was
produced and the nest's appearance remains unverified.

Both launches recorded `UBT AutoSDK ReturnCode: -532462766` (`0xE0434352`),
including the retry that progressed into shader compilation. This confirms the
same managed-exception code can recur during a bounded scratch-editor launch,
but it is not a fatal blocker by itself and does not identify the managed stack
or prove that every desktop popup has this source. The retry's immediate
blocker was the cold, memory-only shader cache. Next visual attempt should use a
writable persistent DDC/cache already populated for the required editor shaders,
then capture only the Wind Arch / West Roost viewpoint. Keep the scratch data
root and the live editor's unsaved state separate.
