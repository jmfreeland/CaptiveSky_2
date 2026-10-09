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
successfully with exit code 0 and no failed assertions. The subsequent visual
check is recorded below.

## Isolated visual-preview and DDC diagnosis

To keep the user's open editor, unsaved materials, and live world state
untouched, the preview was placed in the scratch project's
`WorldData/WorldState/Island.json`, isolated from the main `WorldState`
directory. An initial fixture at the perch marker
`(-100640, 101250, 3079.11)` was only a rough placement: the raven's weave code
traces the actual support surface below the marker. The test log measured that
support 63.2 cm below the marker, so the final three-layer preview uses
`(-100640, 101250, 3015.91)`.

The first offscreen capture attempt exited before map load because the installed
Derived Data Cache graph had no writable node; the log explicitly recommends
`-DDC-ForceMemoryCache`. Retrying with that option and scratch-local cache and
shader-working paths got through DDC initialization, but then spent over a
minute compiling cold PCD3D_SM6 editor shaders. The test never reached map or
viewpoint output. The scratch editor used about 4.3 GiB and its shader workers
about 3.4 GiB while the existing editor remained open; the run was interrupted
through its own command session, and its process tree exited. That attempt did
not produce a screenshot.

Both launches recorded `UBT AutoSDK ReturnCode: -532462766` (`0xE0434352`),
including the retry that progressed into shader compilation. This confirms the
same managed-exception code can recur during a bounded scratch-editor launch,
but it is not a fatal blocker by itself and does not identify the managed stack
or prove that every desktop popup has this source. The retry's immediate
blocker was the cold, memory-only shader cache.

A subsequent run using `-ddc=InstalledNoZenLocalFallback` and a scratch-local
`-LocalDataCachePath` completed the cold compile and populated a persistent
cache. Viewpoint automation passed with exit code 0, showing exactly one nest
from the scratch fixture and making no PIE session or resident/model requests.
The wide Wind Arch capture is useful context, but leaves the nest too small and
partly obscured to judge. A dedicated `04a_RavenNestCloseup` camera was added
to `Config/IslandViewpoints.json`; its second capture places the nest on the
measured support and makes the crossed floor and first rim clearly visible at
close range. The nest is still a small element in the frame, and the
engine-cylinder twigs read as placeholder geometry rather than natural woven
branches. The [close-up frame](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Viewpoints/2026-10-09_040320_h12.0/04a_RavenNestCloseup.png)
is an isolated editor-world preview, not a Game/PIE or final-art validation.

Next: refine the placeholder twig silhouette/material using an available
project branch asset if one fits, then judge it in both this close view and a
normal-distance Game frame. Keep the scratch data root and live editor's
unsaved state separate.
