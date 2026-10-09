# Raven roost shelter shortlist (2026-10-09)

## Question

The bounded physical returns to Roost_East and Roost_West succeeded, but the
Raven's read-only inspections found no local overhead clue and no six-metre
upwind blocker at either site. This follow-up inventories nearby saved-map mesh
bounds to see whether a naturally covered alternative is already present.

## Method and limits

`Scripts/Inspect-IslandRavenRoostShelter.py` loads `/Game/Maps/Island` in an
isolated UE 5.8.3 editor process and reports tagged raven perches plus sizeable
static-mesh bounds that pass within 8 m horizontally and rise at least 1 m above
the marker. It does not move actors, trace candidate surfaces, edit or save the
map, or touch world state. The bounding boxes are only a shortlist: they can
overlap open space, and they do not establish actual rain cover, wind shelter,
branch strength, or usable perch support.

The focused run loaded 180 actors and exited normally. Its UnrealBuildTool
AutoSDK validation returned 0 in this run; this is not evidence about the
source of a separately displayed `0xE0434352` dialog.

## Results

| Existing marker | Plausible overhead bounds within 8 m | Nearest vegetation bounds | Interpretation |
| --- | ---: | --- | --- |
| Roost_West | 1 | Roost_East_Spruce, 7.18 m from the marker's XY bounds | The only overhead-sized vegetation bound is the distant east spruce; it does not indicate a canopy over the ledge. |
| Roost_East | 4 | Roost_East_Spruce overlaps the marker's XY bounds; SaplingA 2.62 m, SaplingB 3.24 m | Bounds suggest nearby tree geometry, but the separate in-world five-point cover check still found 0/5 cover probes at the marker. The broad spruce bound is not proof that its foliage covers this perch. |

The East shortlist also includes a narrow trunk-support proxy 2.35 m away.
Neither it nor the nearby saplings has been tested as an upward-facing perch
surface. The current authored markers therefore remain the only validated
perches, and both are exposed to the measured wind. No alternative sheltered
nest site is established by this audit.

## Runtime foliage audit

The first candidate-grid check ran in the editor world, where the Island's
runtime foliage HISM components may not yet have instances. Its `0/5` result is
therefore not sufficient evidence about spruce crowns. The editor automation now
reports the `IslandSpruce` instance count and explicitly calls canopy results
indeterminate when the component is empty; it never attempts a landing from
that incomplete result.

To get initialized-foliage evidence without saving the map, the scratch Game
build exposes `Island.RavenShelterAudit`. It waits five Game seconds for
resident/ecology startup, verifies live `IslandSpruce` instances, then scans
supported ground points around `Roost_East`. It is read-only by default. The
optional `Land` argument can only request a transient perch when at least three
of five overhead clues are present, the Raven is already settled, and no action
is in progress; the approach result is bounded to 30 simulated seconds. Scan
radius and spacing are clamped (`RadiusCm=1000..30000`,
`GridSpacingCm=200..1500`).

Two isolated UE 5.8.3 Game runs used NullRHI, disabled resident thinking,
`-CaptiveSkyMaxRealtimeSeconds=120`, and
`-CaptiveSkyMaxModelRequests=0`. The local scratch data root kept world-state
writes out of the authored project. Runtime `IslandWeather` had 1,812,124 HISM
instances across 22 components, including 14,864 `IslandSpruce` instances.

| Scan | Supported points | Best overhead clue | Best point | Wind/support | Outcome |
| --- | ---: | ---: | --- | --- | --- |
| 30 m radius, 3 m grid | 316 | 0/5 | 4.44 m from East marker | Landscape support; upwind geometry blocked within 6 m | No landing requested |
| 120 m radius, 8 m grid | 709 | 0/5 | 8.50 m from East marker | Landscape support; upwind geometry blocked within 6 m | `Land` correctly declined by the 3/5 screen |

Both sessions ended normally at 120.1/120.2 real seconds with zero model
requests. The wider result means no sampled supported point within 120 m met
the current local overhead-cover clue; it does not prove the Island lacks
covered branches or a suitable perch at a different elevation. No persistent
map, foliage, Raven, nest, or world-state changes were made.

Runtime logs: `Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Logs/Codex_RavenShelterAudit_Runtime.log` and `Codex_RavenShelterAudit_WideRuntime.log`.

## Unreal/.NET startup exception note

Both Game runs launched from this restricted Codex shell logged
`LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766` while invoking
UnrealBuildTool's `-Mode=ValidatePlatforms` child. That signed return code is
`0xE0434352`, matching the generic CLR exception code in the user's dialog.
Read-only inspection in this shell also confirmed that
`C:\Users\freel\AppData\Local\UnrealBuildTool` is inaccessible here. Running
the same UE 5.8.3 build and focused editor automation with the required
user-level access then completed successfully, including `Win64 VALID`.

This is a strong explanation for the matching code when Unreal is launched
from this restricted tool context, but does not establish the cause of a dialog
from an independently launched desktop editor. The runtime itself continued
past the failed platform-validation child; no matching Windows Application log
entry was available to expose the managed exception's stack trace.

## Next

If Raven is to have a truly sheltered nest location, authoring a new perch or
changing the saved Island layout is likely necessary; the existing East tree
bounds are only a lead, not a validated site. Before that binary Content/map
change, inspect the candidate visually and prove support, overhead cover, wind
exposure, approach clearance, and a bounded Raven landing. No map change is
included in this milestone. The runtime scan suggests a dedicated elevated
branch perch (with explicit collision/support) may be needed; the current
landscape-supported markers cannot stand in for a tree roost.

Log: [`Codex_RavenRoostShelterAudit_Focused_20261009.log`](../../Saved/Logs/Codex_RavenRoostShelterAudit_Focused_20261009.log).
