# Generated-prop placement safety — 2026-10-17

`Scripts/Place-IslandProps.py` previously removed every actor tagged
`GeneratedProp` or `TripoProp` before rebuilding the complete config list, and a
normal invocation saved the level. That made a one-prop RatHollow trial capable
of replacing unrelated user-authored or generated props.

The placement script now previews by default. Applying requires
`PLACE_PROPS_APPLY=1`; `PLACE_PROPS_DRYRUN=1` always wins and remains
non-mutating. After every configured mesh and ground placement passes
preflight, it updates only a single existing managed actor whose exact ID is in
the config, and spawns missing configured IDs. Managed actors with other IDs
and legacy actors not named in the config are left alone. Duplicate matches,
ambiguous actors, invalid IDs, or preflight failures stop before any actor is
changed. The script saves only the current level, not every dirty package.

Seven mocked Unreal-Python checks cover default preview, dry-run-over-apply,
explicit-path resolution, update of the selected ID while unrelated generated
and legacy actors stay untouched, duplicate-ID refusal, and preflight failure.
They pass with the project-local scratch temp root. No Unreal level was opened
or saved for this change, so the Unreal Python API behavior and actual map
appearance still need a bounded editor/commandlet verification.

This removes the placement script's global replacement hazard; it does not
approve a Rat home coordinate. The North Woodland scan's visual candidates
remain unreviewed in the live map, and the existing Rat-height art pass still
shows a blocked entrance. Next, use the explicit preview mode to validate one
selected candidate's ground/slope/approach plan, then run a disposable visual
and capsule traversal check before applying or writing Rat place-memory.
