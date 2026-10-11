# Rat runtime home handoff (2026-10-11)

The Rat's first home is user-selected as a hidden hollow in fallen wood, but no
site has passed both concealment and navigation review. `Agents/Agent_Rat_01/places.json`
currently has an empty `places` array, and the world-state file contains no
canonical Rat home. This change therefore does not place a home or move the
current resident.

`UIslandRatSubsystem` now checks persistent `FIslandWildlifeHomeRecord`s before
using its existing Inn-side spawn. It accepts only a finite, in-range entrance
whose species is Rat, whose occupant is `Agent_Rat_01`, and whose shelter is not
marked Disturbed. Settled homes rank ahead of temporary ones; quality and then
stable ID break ties. The spawner projects the entrance onto nearby navigation
and looks within a 4 m radius. If navigation cannot support that entrance, it
logs the reason and retains the existing 7–12 m Inn-side search. With no
matching home record, behavior is unchanged.

The focused `CaptiveSky2.Agent.RatHomeSelection` automation passed under UE
5.8.3 NullRHI, covering empty records, wrong species/occupant, disturbed homes,
and preference for the best settled home over a higher-quality temporary one.
The editor target also built successfully. Log:
[`Codex_RatHomeSelection_20261011.log`](../../Saved/Logs/Codex_RatHomeSelection_20261011.log).

This verifies selection and compilation, not an Island runtime spawn from an
actual home record: no canonical Rat-home location exists yet, and the editor
visual/navigation review is still required before authoring one.
