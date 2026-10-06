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

The transient Raven now prefers `SK_Crow` when the optional pack is present, maps its idle/hop/takeoff/
fly/landing animations to the existing Raven locomotion states, and retains the procedural fallback.
The imported rig is visual-only for collision and navigation. Current UE 5.8.3 editor-target build,
Raven perch/flight automation, and the real-RHI Raven pose capture pass. The pose test verifies that
`M_Crow` resolves `T_Crow_BaseColor`. A source/UV check found the mesh samples dark albedo (about RGB
66); a representative body pixel is RGB 88 in the deferred Base Color capture but RGB 203, 197, 184
in the final lit capture. Setting only the diagnostic capture's exposure bias to zero brings that
pixel to RGB 159, 149, 130. The material and texture wiring therefore work; the pale gameplay-scale
appearance is coming from the Island's light/exposure response, not missing Crow textures. The actual
light/exposure fix and live-editor visual review remain open, so this is not yet a polished Raven.
The diagnostic images are in `Saved/Viewpoints/RavenWingMotion_/20261006_141548/`; the copied content
remains in ignored `Content/`, outside Git.

An A/B check of the proposed project-wide exposure change used matched 21:00 Island captures with
the original bias `+1` and experimental bias `0`. Zero bias makes the grass and shoreline nearly
black and hides most ground detail; `+1` preserves readable vegetation, water, and cloud detail.
The project default is therefore restored to `+1`. This rules out a global exposure change as the
Raven fix: continue with a localized Crow material or light response that does not darken the whole
night landscape. Paired captures: `Saved/Viewpoints/2026-10-06_142822_h21.0/` (zero) and
`Saved/Viewpoints/2026-10-06_143348_h21.0/` (+1).
