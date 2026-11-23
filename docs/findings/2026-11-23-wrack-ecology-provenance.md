# Tidepool wrack event provenance — 2026-11-23

The tidepool crab's existing, bounded wrack-investigation detour now appends one spatially sourced Chronicle event when it begins: `type=wildlife`, `event=crab_wrack_investigation`, a stable `source_id`/`wrack_id`, and the materialized wrack position in `at`. A resident's successful `Examine` records the related `wrack_turned` event as `shore_find_discovered`, with the same source ID, location, and discovered fact. The records make the observed sequence inspectable without giving residents omniscient knowledge or adding model calls.

The offline World Atlas now accepts `wildlife` Chronicle entries and retains bounded `source_id` values in map-marker and timeline provenance. `Build-Chronicle.py` includes both wildlife investigation and wrack-discovery records in its readable history. Existing gameplay timing, movement, inventory, and save behavior were not changed.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` compiled and linked successfully (9 UBT actions; 35.19 seconds), compiling the crab, wrack, and focused automation-test sources.
- `python -m unittest Scripts.tests.test_build_chronicle Scripts.tests.test_build_world_atlas`: 11 tests passed, including string-coordinate mapping and source-ID visibility.
- The focused Unreal command-line automation attempt ended after the AutoSDK validation output and did not create `Saved/Logs/Codex_WrackProvenance_20261123.log`; no pass is claimed for the automation fixture or live detour. No Unreal editor process remains running.
- No Island map, canonical WorldState, runtime Content asset, or resident model-call pathway was changed.

This adds provenance to the existing transient crab cue and verified resident inspection record. It does not yet make a discovered object a separate materialized prop, store witness identities, or make residents learn the event through conversation.
