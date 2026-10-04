# Free animal assets: crow and wolf (2026-10-04)

The free Fab [Animal Variety Pack](https://www.fab.com/listings/2dd7964c-a601-4264-a53d-465dcae1644c)
is available in the neighboring `Mountain_v00 5.5 5.6` project. The project stores the assets under
`Content/AnimalVarietyPack`. Fab lists Unreal Engine 5.0–5.5 compatibility and a July 2024 update;
CaptiveSky uses UE 5.8.3, so asset-thumbnail loading is an initial compatibility check, not proof
of packaged-game or runtime compatibility.

Only the Crow and Wolf source folders were copied into CaptiveSky's ignored `Content/`, after
confirming neither target folder existed. This is 74 packages totaling 158,357,321 bytes (about
151 MiB):

| Subset | Packages | Source size | Asset registry / preview checks |
| --- | ---: | ---: | --- |
| Crow | 27 | 25,193,632 bytes | `SK_Crow` loads and renders; 2,208 triangles, 61 bones, one material slot; idle-look, hop, fly, landing, and takeoff sequences resolve to its skeleton. |
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
reversible fallback. Any Wolf integration should first decide whether it is ambient wildlife or a
named, conscious companion; only the latter needs a resident memory/personality directory and
agent-request budget. Keep its population bounded and preserve the shared request safeguards.
