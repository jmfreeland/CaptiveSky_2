# Frog attention-window readability — 2026-10-28

The Common Frog's low-hovering dragonfly response now holds its turn for 2.4
seconds instead of 1.4. That gives a visitor more time to notice the glance
and connect it to the passing Tideglass dragonfly. The existing 16-second
cooldown, visibility/range checks, single-pass presence latch, and hop/startle
priority are unchanged, so this does not increase actor count, polling cadence,
model requests, or saved state.

The focused `CommonFrogDragonflyAttention` automation test now checks that the
frog is still tracking after 1.5 seconds, then returns to its resting heading
after the cue expires. UE 5.8.3 successfully compiled and linked the updated
frog and test in the existing isolated scratch project. An unattended
`UnrealEditor-Cmd` automation invocation returned without producing a fresh
log or a test result, so runtime automation remains inconclusive. The cue is
also visually unverified in the Island. Validate it in a bounded
zero-model-request Tideglass session when the project can be opened; if it reads
as lingering rather than attentive, tune the duration back down.
