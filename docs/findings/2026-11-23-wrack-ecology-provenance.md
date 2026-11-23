# Tidepool wrack event provenance — 2026-11-23

The tidepool crab's existing, bounded wrack-investigation detour now appends one spatially sourced Chronicle event when it begins: `type=wildlife`, `event=crab_wrack_investigation`, a stable `source_id`/`wrack_id`, and the materialized wrack position in `at`. A resident's successful `Examine` records the related `wrack_turned` event as `shore_find_discovered`, with the same source ID, location, and discovered fact. The records make the observed sequence inspectable without giving residents omniscient knowledge or adding model calls.

The offline World Atlas now accepts `wildlife` Chronicle entries and retains bounded `source_id` values in map-marker and timeline provenance. `Build-Chronicle.py` includes both wildlife investigation and wrack-discovery records in its readable history. Existing gameplay timing, movement, inventory, and save behavior were not changed.

The human-readable Chronicle also includes the bulk `wrack` event for storm-deposited shore material. That summary has no exact coordinates; the Atlas keeps it in the timeline without placing it on the map. This formatter/Atlas integration follow-up was made after the verification below and has not been retested.

## Verification

- UE 5.8.4 `CaptiveSky_2Editor` compiled and linked successfully (9 UBT actions; 35.19 seconds), compiling the crab, wrack, and focused automation-test sources.
- `python -m unittest Scripts.tests.test_build_chronicle Scripts.tests.test_build_world_atlas`: 11 tests passed, including string-coordinate mapping and source-ID visibility.
- UE 5.8.4 `CaptiveSky2.Agent.TidepoolCrabShoreForage` automation passed with exit code 0. The log records the new crab-investigation source ID/location assertions and the resident `shore_find_discovered` record assertion: `Saved/Logs/Codex_WrackProvenanceRetry_20261123.log`.
- The same command-line run loaded Island and completed its map check with 0 errors and 0 warnings. This confirms startup and the focused fixture, not a live gameplay/visual session.
- No Island map, canonical WorldState, runtime Content asset, or resident model-call pathway was changed.

This adds provenance to the existing transient crab cue and verified resident inspection record. It does not yet make a discovered object a separate materialized prop, store witness identities, or make residents learn the event through conversation.
