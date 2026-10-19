# Sea-glass storm wrack

## Design

Storm deposits can now rarely leave a scatter of surf-rounded green-blue sea glass. It is selected deterministically from the existing storm seed (5% of deposits), appears as three to five flattened, irregular chips, has a cooler and slightly glossier material response than shells or kelp, and remains in the shore ledger for 18 Island days. Residents can use the existing `interact`/turn-over path; deterministic findings include a sheltered tidepool periwinkle, a sandhopper, shore-crab tracks, or an old maker's mark.

The new kind is appended after the original four enum values and stored under the `sea_glass` JSON name. The ledger version and existing serialized names are unchanged, so old saved wrack remains readable. The chips use the existing basic-shape mesh/material path, have no collision, and do not create model requests or modify the Island map. No imported content was added.

## Verification

- `git diff --check` passed for the implementation and test files.
- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded. The first build compiled `IslandWrack.cpp` and `IslandWrackTests.cpp` and linked the module (10 actions, 54.93 s). After adding the specific periwinkle-discovery assertion, the incremental build compiled `IslandWrackTests.cpp` and linked successfully (4 actions, 5.01 s).
- The focused automation test covers rarity, deterministic descriptions, the shard layout and color, a periwinkle discovery, ledger round-trip, and the existing four kinds. It compiled but was not run: the project-startup null-access crash remains unresolved, and the in-editor Unreal MCP bridge is unavailable. No additional Unreal project process was launched.
- In-world appearance, storm timing, and discovery readability remain unverified. Review sea glass after a natural or controlled storm when the current editor session can safely run the focused test; preserve the existing capped-play and zero-model-request safeguards.
