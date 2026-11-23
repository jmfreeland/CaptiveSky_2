# World atlas snapshot

`Scripts/Build-WorldAtlas.py` builds a self-contained HTML/SVG atlas from explicitly supplied, stopped-game copies of Island world state, trail history, and chronicle data. It is an offline reading tool: it does not connect to Unreal, read live actors, call a model, or write canonical `WorldState` files.

The page maps recorded trail cells and stored place coordinates, then lists chronicle events that carry spatial provenance. It supports day and region filters for recorded events, bounded input sizes/counts/coordinates, and HTML escaping. It recognizes supported world/trail schema versions and reports corrupt core inputs; malformed or unsupported chronicle records are skipped. Optional anchors must be supplied explicitly from a source the caller trusts.

Commit `4a22f7ec` adds a proportional 1/2/5 distance scale to the map when spatial evidence is present. A snapshot with no spatial evidence explicitly reports that a scale is unavailable; it does not derive a scale from the empty-map placeholder bounds.

The page also summarizes the supplied trail-cell count, sum of per-cell steps, saved place-record count, and chronicle events with a spatial region. These are snapshot totals only; they are not live counts or a reconstruction of prior activity.

## Validation

- `python -m unittest Scripts.tests.test_build_world_atlas`: 7 tests passed, covering empty and legacy snapshots, negative coordinates, chronicle coordinate strings, bad and oversized input, corrupt world state, escaping, and event provenance.
- `git diff --check`: clean.
- A local preview was generated at ignored path `Saved/WorldAtlas/Island.html` from copies under `Saved/WorldAtlas/Input/`. Those source files are historical: the newest recorded chronicle data is from 2026-11-01. The captured-at label in the generated page discloses the copy date and source timestamp.
- The listed test run and preview predate the scale-bar and snapshot-summary changes; no tests or browser review have been run for those additions. Review the scale size/label across representative map extents and the summary on narrow screens when the shared computer is available.

This is a static snapshot, not a reconstructed historical map: trail cells show what the copied trail file contains, and day filters apply to event markers/listing. No visual browser review was completed while the user was using the shared computer.
