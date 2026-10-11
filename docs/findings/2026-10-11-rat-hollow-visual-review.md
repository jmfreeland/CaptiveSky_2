# Rat Hollow prop review

## Current asset

- Unreal asset: `/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01` (StaticMesh).
- Source and import report: `Saved/CaptiveSky/ComfyBlender/RatHollow/`; `unreal_import.json` reports no validation errors.
- Unreal registry: 1,602 vertices, 1,836 triangles, four material slots (wood, dark hollow, straw, moss), four simple box collision shapes, two UV channels, Nanite disabled; approximate bounds 256 × 136 × 134 cm.
- Read-only Static Mesh Editor inspection shows a weathered fallen log and a clearly distinct circular hollow entrance; the source asset also includes moss patches and a small straw nest bed. The default side thumbnail hides the entrance; orient it toward the Rat's approach/habitat when placing it.

## Relationship to Fenrus

`Agents/Agent_Rat_01/identity.md` says the Rat's first home is a hidden fallen-wood hollow. `Agents/Agent_Rat_01/places.json` is still empty, so no in-world coordinates are claimed yet. The asset is imported and visually readable, but has not been placed or traversability-tested with a Rat body.

The user has confirmed the home concept: a hidden fallen-wood hollow. This confirms the habitat/design, not a particular Island site; keep `places.json` empty until a sheltered, dry, accessible location is verified in-world.

A read-only live viewport survey at `(-100500, 102000)` showed open grass beside the inn with little nearby cover; a higher survey near `(-100000, 101000)` showed only a sparse tree group. Reject this near-inn patch as a hidden home candidate. The island needs more understory/vegetation before a convincing sheltered rat home can be selected; no RatHollow placement was made.

## Placement safety

The live `/Game/Maps/Island` and `/Game/Generated/ProceduralVegetation/PVE_Deciduous_Shrub_01` currently report dirty/unsaved in Unreal. This inspection did not edit or save either. Do not run `Scripts/Place-IslandProps.py` against the current session: a normal run replaces every actor tagged `GeneratedProp`/`TripoProp` and saves the level. First preserve the current authoring session, then choose a hidden, dry, Rat-accessible ground point, validate the approach/orientation and collision, and only then create the home placement and update Rat place-memory.

The bounded read-only `Island.RatHollowSiteAudit` command is implemented in `Source/CaptiveSky_2/Agent/IslandNavProbe.cpp` (commit `c696305`) and only runs in a Game world. The current UnrealEditor process started before that source change, so its loaded module does not yet prove the command is available; `IsPIERunning` is false. Do not start another editor/build beside the current session: it still owns unsaved Island and shrub work and the machine has roughly 4 GiB of available memory. After that work is safely preserved and the updated module is loaded, run the site audit in one bounded diagnostic session, visually inspect its suggestions, and physically test the Rat's approach before recording coordinates. The scan itself does not save or modify the world and its output is only a site suggestion, not placement approval.
