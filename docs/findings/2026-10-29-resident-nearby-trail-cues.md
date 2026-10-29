# Resident nearby trail cues

## Change

Residents now receive a small, local description of up to two nearby worn trail regions in their environment summary. Each clue is derived from the step-weighted centre and shape of recorded worn cells, and includes approximate bearing, distance, patch count, and whether the trail is recent or recovering.

The query is bounded to an 81-region neighbourhood and a 45 m centre-distance radius. It excludes the resident's current region, which is already described by the underfoot cue. The description explicitly identifies these as clues to past foot traffic rather than a verified route or destination. Reading the ledger does not mutate it.

## Coverage

`IslandTrailTests` now covers bearing and alignment, the route/destination caveat, exclusion of the underfoot region, the two-region cap, distance cutoff, empty ledgers, read-only behavior, and invalid/out-of-range coordinates.

`git diff --check` passes. The UE 5.8 editor target compiled and linked in this worktree using UnrealBuildTool directly (10 actions, 66.70 seconds), including the new test translation unit. The batch wrapper's location guard still fails in this environment, but bypassing it allowed the normal target build to complete. The automation tests remain unrun: the connected editor is using a different project build, and the previously documented headless Unreal startup crash makes another identical test launch uninformative. No PIE session was started, in keeping with the current GPU headroom concern; the editor and its pending material edit were left untouched.
