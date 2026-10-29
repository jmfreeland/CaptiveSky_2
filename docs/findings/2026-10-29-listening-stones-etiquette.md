# Listening Stones remembered etiquette (2026-10-29)

## Behavior

An autonomous resident can learn the quiet-listening practice only by directly
seeing another resident remain quietly at the Listening Stones; the existing
memory is tagged `practice:quiet-listening`. This change makes that memory
actionable without an extra model call: when the learner chooses `move_to`
ListeningStones while another resident is still quietly listening, it projects
a wait point 8.5 m beyond the landmark on the listener's far side. The move is
accepted only when navigation finds a complete path and the character capsule
can travel it without collision. If no such route exists, the learned resident
waits or chooses another activity rather than taking the ordinary centerward
approach. A resident does not yield to itself, residents without the witnessed
practice keep their existing choice, and the current quiet visitor is not
displaced.

This is a one-resident, local etiquette cue, not a global culture score. It
does not alter landmark state, conversation rules, the quiet visitor's
behavior, persistence formats, or model-request pacing. The existing
Listening Stones presentation remains the source of truth for whether a
resident is currently quietly visiting.

## Validation

The focused deterministic automation test covers the memory gate, self-visitor
case, far-side geometry, ground-plane preservation, and degenerate fallback.
Unreal Header Tool completed, and UnrealBuildTool compiled the changed
controller, presentation dependency, and new test translation unit. The main
project module could not be linked: the editor process holds
`Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` open. Therefore the automation
test and in-world arrival have **not yet been run**. Do not terminate the open
editor to release the file; retry the incremental link and focused test after
the editor is closed or use a separate-output worktree build.

## Follow-up

After the focused test passes, run one short, thinking-disabled PIE scenario
with two residents: the witness chooses ListeningStones while the other stays
quiet there. Confirm the witness reaches the far edge, no approach chime is
triggered for the learner's arrival, and the quiet resident remains in place.
Keep the existing bounded play/request limits enabled and verify zero
additional model requests from the etiquette behavior.
