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

## Tapered twig silhouette and shadow pass (2026-10-09)

No branch mesh was available in the project content search. A visual-only
placeholder refinement now pairs each cylinder twig with two short,
site-seeded cone tips. The body, tip, and storm-debris components remain
collisionless and off navigation; twig meshes no longer cast hard shadows
across the small rock perch. The seed and instance transforms remain stable
when the same nest is rebuilt. This does not add shelter, support, or new
persistent state.

The `CaptiveSky2.Agent.IslandNest` automation passed in the UE 5.8.3
scratch editor with model requests disabled and a 60-second realtime cap. It
checks opposing tip orientation, visible extension past the twig bodies,
deterministic reconstruction, collision/navigation policy, storm damage, and
shadow policy. The `CaptiveSky2.Visual.Viewpoints` test also passed with a real
RHI, scratch world-state root, and only the RavenNestCloseup view. The scratch
copy of that viewpoint was moved closer solely to judge the silhouette; this
iteration did not edit the project camera. The [zoomed preview](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Viewpoints/2026-10-09_043134_h12.0/04a_RavenNestCloseup.png)
shows the weave without the previous hard black nest-shadow cluster. The
remaining brown rods are still engine primitives, not natural branch art; a
proper twig mesh is the next art-quality step if an available/free asset fits.
The user's open editor and its unsaved materials were left untouched.

## Primary-world persistence check (2026-10-09)

The current primary `WorldState/Island.json` has an empty `nests` array. Raven's
saved `memory.jsonl` contains 879 records dated 2026-08-22 through 2026-09-30;
the only nest/twig reference is an earlier Discord conversation expressing a
wish for a quiet place to nest, with no memory of foraging or weaving. The
separate `places.json` remembers only the Cairn. This explains why the 17:00
Game screenshot's `04a` view showed the physical Roost_West rock/perch rather
than a woven nest: there is no nest record in that world to materialize. The
close-up is therefore a misleading camera title, not evidence that the nest
actor failed to render.

In ordinary play, `GatherTwigs` is offered only after the Raven lands at a
visible forage patch, and weaving is offered only while carrying twigs at a
verified perch. Both actions remain optional; the system does not assign a
home or create a nest automatically. Do not seed a nest into the user's save or
force the Raven to build merely to improve a screenshot. The next meaningful
check is a separately approved, short live observation with the normal model
request ceiling in place, asking whether the Raven discovers and chooses this
multi-step project over time. Until then, an empty nest list is an honest
world-state result, and the generated close-up should not be presented as a
portrait of an existing nest.

## First capped live-autonomy observation (2026-10-09)

A standalone Game run used an isolated data root, a 300-second real-time ceiling,
a six-model-request ceiling, disabled Python, and a warm local memory DDC. The
Island became ready after 39.8 seconds. Play ended itself after 81.9 real seconds
at exactly six requests, with no requests still in flight and a normal Unreal
shutdown. The primary world's state and resident memories were not used as the
write destination:
[`RavenAgency.log`](../../Saved/Playtests/Codex_RavenNestAgency_20261009_retry/RavenAgency.log)
and the isolated
[`World/`](../../Saved/Playtests/Codex_RavenNestAgency_20261009_retry/World/).

The first model-driven choices were not scripted toward the nest. Raven
approached a Tideglass dragonfly, then considered the rain basin and chose to
watch the dragonfly. Aster chose a gentle approach toward Raven, then created an
eight-stone arrangement at `ArrangingGround_1`, titled “A Pause Between.” The
Innkeeper also moved independently. This is direct evidence of varied curiosity
and small, persistent world-making within the request cap. It is not evidence
that Raven rejected nesting: this run started with a fresh isolated memory
store, so his existing 879 primary-world records—including the earlier recorded
nest wish—were not in his prompt. He did not gather twigs or weave during the
six requests.

Do not copy Aster's isolated arrangement or seed a nest into the primary save.
The next useful agency check is a separately bounded isolated run with a
read-only copy of the existing memories, preserving the normal choice to ignore
nesting. Before spending more model requests, first verify the old wish can be
retrieved in the actual nearby-roost situation; retrieval uses a 24-hour
recency half-life, so a September conversation may be crowded out by newer
experience even when the scene mentions a nest.
