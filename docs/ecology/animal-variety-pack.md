# Free animal assets (2026-10-04)

The free Fab [Animal Variety Pack](https://www.fab.com/listings/2dd7964c-a601-4264-a53d-465dcae1644c)
is available in the neighboring `Mountain_v00 5.5 5.6` project. The project stores the assets under
`Content/AnimalVarietyPack`. Fab lists Unreal Engine 5.0–5.5 compatibility and a July 2024 update;
CaptiveSky uses UE 5.8.3, so asset-thumbnail loading is an initial compatibility check, not proof
of packaged-game or runtime compatibility.

Crow and Wolf were copied from the neighboring `Mountain_v00 5.5 5.6` project; Deer, Fox, and Pig
were copied from the neighboring CaptiveSky UE 5.8 project. All are in CaptiveSky's ignored
`Content/AnimalVarietyPack/`, after confirming each target folder was absent. The complete pack is
220 packages totaling 517,854,968 bytes (about 494 MiB):

| Subset | Packages | Source size | Asset registry / preview checks |
| --- | ---: | ---: | --- |
| Crow | 27 | 25,193,632 bytes | `SK_Crow` loads and renders; 2,208 triangles, 61 bones, one material slot; idle-look, hop, fly, landing, and takeoff sequences resolve to its skeleton. |
| Deer stag + doe | 65 | 160,834,727 bytes | `SK_DeerStag` and `SK_DeerDoe` load; 6,774 and 6,294 vertices respectively, each has four LODs and two material slots. Animations include grazing, walking, running, resting, sleeping, and getting up. |
| Fox | 39 | 30,471,877 bytes | `SK_Fox` loads; 5,658 vertices, four LODs, two material slots. Animations include looking around, resting, running, biting, and jumping attacks. |
| Pig | 42 | 168,191,043 bytes | `SK_Pig` loads; 6,588 vertices, four LODs, two material slots. Animations include looking around, sniffing, chewing, resting, and running. |
| Wolf | 47 | 133,163,689 bytes | `SK_Wolf`, skeleton, physics asset, and animation sequences load; 10,105 triangles and 34 bones. Idle-look (10 s), walk (1 s), run (0.53 s), and sleep sequences are present. |

Fresh UE 5.8.3 `CaptureAssetImage` renders show visible coloration on all five rigs. New captures of
`SK_DeerDoe`, `SK_Fox`, and `SK_Pig` on 2026-10-04 resolve the earlier grey-thumbnail discrepancy;
no material reassignment was needed. These isolated real-RHI asset previews confirm that the meshes
and assigned materials load and render, but do not establish their appearance at gameplay scale,
under Island lighting, or in a packaged build. The dependency walk reports duplicate legacy texture
paths outside `/Game/AnimalVarietyPack/` for several pack materials (including `/Game/Crow/Textures` and
`/Game/Wolf/Textures`). No linker or missing-object warning appeared during the earlier Crow/Wolf
loads; review those duplicate references if runtime warnings appear.

The first wildlife source prototype is one transient, non-conscious stag near Wind Arch. Its
graze/walk/run/sleep sequences resolve to the same `SK_DeerStag_Skeleton` as the mesh; all four
have root motion disabled, so movement is deliberately bounded and driven by the actor rather than
animation displacement. It wanders within a 7 m home radius, rests on the existing day/night clock,
and offers only a quiet, reversible observation response. `IslandLife`
wildlife is not a resident movement target for either Aster or the Raven; no LLM calls, persistent
facts, or ownership are involved. The automation fixture is in
`Source/CaptiveSky_2/Agent/Tests/IslandForestStagTests.cpp`.

On 2026-10-05 the current isolated UE 5.8.3 scratch module
(`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Binaries/Win64/UnrealEditor-CaptiveSky_2.dll`,
timestamp 16:09) passed `CaptiveSky2.Agent.WoodlandDeer` with a separate 45-second play cap, zero
model requests, `-NullRHI`, and an isolated data root. The fixture verifies the imported mesh and
four matching animations, disabled root motion and collision, wildlife/target eligibility, respectful
inspection, a startle response bounded to the 7 m home radius, and reversible night rest. Log:
[`Codex_WoodlandDeer_20261005.log`](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_WoodlandDeer_20261005.log).
UE/EOS also emitted blocked outbound-connectivity warnings during startup; the automation still
completed, and the explicit zero-model-request cap remained in force.
This proves the actor and interaction fixture in the current scratch build, not its appearance or
spawn location in the saved Island. The procedural Raven remains intact. Next, compare the rigged Crow
against the procedural Raven at gameplay scale while preserving the Raven's locomotion states and
reversible fallback. If any animal later becomes a conscious named resident, give it an explicit
memory/personality directory and request budget. Preserve the shared request safeguards.

The main editor module remains older than source:
`Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` is timestamped 2026-10-04 19:11. Do not treat automation
run inside that editor as current-source evidence. `CaptiveSky2.Agent.NightEcology` previously failed
in the stale module at the cloud-material assertion; a fresh run against the current scratch module
passed on 2026-10-05 (see the [regression note](../findings/2026-10-04-night-ecology-regression.md)
and [run log](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_NightEcology_20261005.log)). The current test also passes its
Tideglass/Listening Stones firefly-placement checks. The earlier failure is resolved for current
source, but no actor has been placed in or visually validated on the saved Island. The test's map
fixture includes `VolumetricCloud_0` with
`/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst`. Cleanup restores the authored
material after probing.

Earlier (2026-10-04), UHT succeeded against a scratch editor manifest and VS 2022 compiled eight
current translation units to scratch objects, including `IslandForestStag.cpp`, its test,
`IslandWeather.cpp`, and related interaction/prompt sources. A later isolated module link was
completed for the 2026-10-05 scratch tests described above. Those tests validate the stag fixture and
personality consolidation, not the weather spawn path, cloud pass, live model behavior, target-hardware
performance, or visual quality. Next, run the ecology/weather suite against a current isolated build,
then inspect the stag and cloud behavior in the live Island before comparing Crow and Raven at
gameplay scale.

## Rigged Raven visual milestone (2026-10-06)

The transient Raven prefers a local `SK_Crow_CaptiveSky` copy when available, then falls back to
`SK_Crow`; it maps the idle/hop/takeoff/fly/landing animations to existing locomotion states, and
retains the procedural fallback. The imported rig is visual-only for collision and navigation. A
separate `M_Crow_CaptiveSky` multiplies only the source Base Color, and the copied mesh assigns it in
its default material slot; the Fab source assets remain untouched. The current UE 5.8.3 editor-target
build and `CaptiveSky2.Visual.RavenWingMotion` pass, including assertions for the selected mesh and
material, but the capture does **not** prove the new material is rendered.

Earlier SceneCapture diagnostics showed that matched isolated-Raven images before and after changing
the saved `FeatherAlbedoScale` from `0.22` to `0.025` were byte-identical (body pixel RGB 202, 196, 184);
a runtime dynamic-material override to zero and a Base Color pass also left the same body pixel/image.
Hiding the Raven body did remove it, so the capture was observing the transient mesh, but it appeared
to ignore material changes. An added static-sphere control reported the same custom material parent,
yet its pixel was RGB 180, 172, 155 for both zero and 0.025 albedo. Replacing that sphere's material
with the engine `WorldGridMaterial` and recreating its render state still produced RGB 181, 173, 156.
This suggests the SceneCapture-based automation path is not a trustworthy material-binding check. Asset audit
confirms the copied mesh slot points to
`M_Crow_CaptiveSky` and the material graph contains the expected multiply. The copied mesh's only
LOD-0 render section uses material index 0, and the component resolves that index to
`M_Crow_CaptiveSky`, so a bad section index is not the cause. These SceneCapture results were
inconclusive about the lit game appearance; the normal game-render check is recorded below. Diagnostic
captures are under
`Saved/Viewpoints/RavenWingMotion_/20261006_154133/`, `.../20261006_154810/`,
`.../20261006_155439/`, `.../20261006_155643/`, `.../20261006_155909/`, and
`.../20261006_160225/`, `.../20261006_160731/`, `.../20261006_162043/`,
`.../20261006_162327/`, `.../20261006_162602/`, and `.../20261006_162940/`; copied assets remain
in ignored `Content/`, outside Git.

The first bounded Game capture on 2026-10-07 reached the Island and confirmed the copied mesh,
material, and dark feather appearance in a normal game render. It also exposed a spectator-camera
bug: the Raven can fall or walk away from the actor-tag location after viewpoints are loaded, while
later cuts kept using the original absolute camera points. The initial four-angle sweep therefore
showed the Raven in only the first image. `FIslandShot` now tracks a shared tagged anchor, rebases its
camera/look-at/focus points when each shot begins, and carries the camera and hidden visitor with the
anchor on subsequent ticks. `CaptiveSky2.Agent.Spectator` verifies both movement before a cut and
movement during a shot. The UE 5.8.3 editor target builds successfully, and the test passes.

A second 25-second standalone Game run (thinking disabled, one-request maximum, isolated world
state) exited normally after 25.3 seconds with zero model requests. All four 1600x900 frames show
the dark Crow from the East, West, North, and South viewpoints; they are in
`Saved/Playtests/Codex_CrowVisual_20261007/ScreenshotsAnchorFollow2/`. The Raven now reads as a dark
bird in the actual game. The red texture-streaming pool warning is still visible in these captures;
remove that diagnostic overlay before selecting a polished highlight frame. The earlier
`ValidatePlatforms` launch stall did not reproduce in the successful editor build and Game run.

The earlier source/UV check found the Crow mesh samples dark albedo (about RGB 66); the original
material's representative body pixel was RGB 88 in a deferred Base Color capture but RGB 203, 197, 184
in the final lit capture. Setting only the diagnostic capture's exposure bias to zero brought that
pixel to RGB 159, 149, 130. A project-wide exposure A/B found that lowering the default from `+1` to `0`
makes the grass and shoreline nearly black, so retain `+1` and pursue a localized Raven fix. Paired
captures: `Saved/Viewpoints/2026-10-06_142822_h21.0/` (zero) and
`Saved/Viewpoints/2026-10-06_143348_h21.0/` (+1).
