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

## Next

If Raven is to have a truly sheltered nest location, authoring a new perch or
changing the saved Island layout is likely necessary; the existing East tree
bounds are only a lead, not a validated site. Before that binary Content/map
change, inspect the candidate visually and prove support, overhead cover, wind
exposure, approach clearance, and a bounded Raven landing. No map change is
included in this milestone.

Log: [`Codex_RavenRoostShelterAudit_Focused_20261009.log`](../../Saved/Logs/Codex_RavenRoostShelterAudit_Focused_20261009.log).
