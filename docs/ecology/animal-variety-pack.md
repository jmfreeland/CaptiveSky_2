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
`Source/CaptiveSky_2/Agent/Tests/IslandForestStagTests.cpp`. The source and fixture now compile to
scratch objects, but have not been linked into or run by the editor; no actor has been placed in or
saved to the map. The procedural Raven remains intact. Next, link a current module and validate this
class and spawn path in UE 5.8.3, then compare the rigged Crow against the procedural Raven at gameplay
scale while preserving the Raven's locomotion states and reversible fallback. If any animal later
becomes a conscious named resident, give it an explicit memory/personality directory and request
budget. Preserve the shared request safeguards.

The current editor tests are definitively against an old module:
`Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` is timestamped 2026-10-01 09:18, while the ecology
test source is from 2026-10-04 04:50 and the
wildlife/weather source is from 2026-10-04 07:17 onward. In that stale module,
`CaptiveSky2.Agent.NightEcology` fails at the assertion that weather creates a transient dynamic
cloud-material instance; the map has `VolumetricCloud_0` with
`/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst`. The test's cleanup restores the
authored material after probing. `CaptiveSky2.Agent.SessionSafety` passed in the same stale module,
so neither result validates the current source.

A fresh UE 5.8.3 build was attempted in an isolated project copy under
`Saved/CompileScratch/Codex_DeerBuild_20261004/Project`, leaving the open editor and its binary
untouched. UBT produced no diagnostics or intermediate files after five minutes, and its launched
process remained idle; that process was stopped. An older unidentified `dotnet` process remains, so
the build mutex was not bypassed and no other process was touched. Rebuild and rerun both tests, then
inspect the weather pass before changing the cloud asset or weakening the assertion. The stag test
and its runtime spawn path remain unverified until a current-source build is available.

On 2026-10-04, UHT was run against a scratch copy of the editor manifest with the new stag header
added and all project-module generated output redirected to `Saved/CompileScratch/Codex_UHT_20261004`.
It succeeded in 94 seconds, leaving the editor's generated files, binary, and map untouched. VS 2022
then compiled eight current translation units to scratch objects using that fresh `IslandWeather`
reflection output plus existing generated headers: `IslandForestStag.cpp`, its automation test,
`IslandWeather.cpp`, `IslandEcologyTests.cpp`, `IslandInteractionUtility.cpp`,
`AgentBrainComponent.cpp`, `AutonomousAgentAIController.cpp`, and `RavenAgentAIController.cpp`. This
resolves the stale-UHT barrier for source compilation, but it is not a full module link and does not
prove runtime behavior. The already-open editor still has the 2026-10-01 module; do not use its
automation results as current-source evidence. Next, link a current module in isolation and run the
stag/ecology tests without changing the live map or replacing the editor DLL.
