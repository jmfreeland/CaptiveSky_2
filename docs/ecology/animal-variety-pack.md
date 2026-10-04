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

The Crow's default material renders as a dark, legible bird in the asset thumbnail. The Wolf's
skeletal-mesh thumbnail appears untextured grey, although its two material slots resolve and the
`M_Wolf` material thumbnail renders its base texture; confirm the full character appearance in a
real-RHI scene before using it. The asset dependency walk also reports a set of duplicate legacy
texture paths at `/Game/Crow/Textures` and `/Game/Wolf/Textures` that are not present in this
project; no linker or missing-object warning appeared in the editor log during these loads, but the
references warrant review before integration.

No animal actor, map placement, AI, or agent identity/personality was added in this pass. The
procedural Raven remains intact. The next visual prototype should compare the rigged Crow against
the procedural Raven at gameplay scale, preserving the current Raven's locomotion states and
reversible fallback. A first ecosystem pass should use a small, bounded population of ambient
wildlife with no LLM calls; if any animal later becomes a conscious named resident, give it an
explicit memory/personality directory and request budget. Preserve the shared request safeguards.
