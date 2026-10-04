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

Current UE 5.8 asset-thumbnail captures show visible coloration on the Crow, stag, and Wolf meshes;
the doe, fox, and pig mesh thumbnails appear monochrome grey. Their skeletal meshes still report
the expected two material assignments, and separate thumbnails of `M_DeerDoe`, `M_Fox`, and `M_Pig`
show textured material previews. This discrepancy makes the thumbnail result inconclusive for the
doe/fox/pig: inspect them in a real-RHI level before integration rather than reassigning materials
blindly. The dependency walk reports duplicate legacy texture paths outside
`/Game/AnimalVarietyPack/` for several pack materials (including `/Game/Crow/Textures` and
`/Game/Wolf/Textures`). No linker or missing-object warning appeared during the earlier Crow/Wolf
loads; review those duplicate references if runtime warnings appear.

No animal actor, map placement, AI, or agent identity/personality was added in this pass. The
procedural Raven remains intact. The next visual prototype should compare the rigged Crow against
the procedural Raven at gameplay scale, preserving the current Raven's locomotion states and
reversible fallback. A first ecosystem pass should use a small, bounded population of ambient
wildlife with no LLM calls; if any animal later becomes a conscious named resident, give it an
explicit memory/personality directory and request budget. Preserve the shared request safeguards.
