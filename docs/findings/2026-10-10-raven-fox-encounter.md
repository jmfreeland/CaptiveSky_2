# A quiet Raven–fox encounter (2026-10-10)

The settled Raven can now acknowledge an awake fox's nearby foraging without
turning a wildlife cue into a task. Within 5 m, with clear sight and a small
height difference, the Raven briefly looks toward an eligible fox; it does not
move. The fox looks back, pauses, then retreats a short distance away while
remaining inside its 6.5 m home patch. A resting or waking fox is ignored.
Existing stag recognition remains in the same nearest-wildlife selection, and a
nearby group is acknowledged only once until it leaves the Raven's 14 m forget
radius. The older 7 m fixture wording was stale; the current shared wildlife
selection uses the same 14 m forget range for both stag and fox. Higher-priority
attention cues still take precedence.

This is a transient, nonverbal wildlife response: it adds no model call,
conversation, memory, or persistent world state. It does not make the fox a
conscious character or a companion with its own identity.

## Verification

- UE 5.8.3 editor target built in the isolated
  `Saved/NavBoundsTest/ProjectCurrent` scratch project.
- `CaptiveSky2.Agent.RavenPerch` passed with the Island map loaded, NullRHI,
  Python and agent thinking disabled, a zero model-request limit, and a
  120-second cap. The regression covers the exact 5 m boundary, resting-fox
  exclusion, reciprocal look, delayed retreat, local-patch bound, and the
  Raven remaining settled.
- This was deterministic automation, not a rendered encounter or a claim about
  how often the animals naturally meet during ordinary play.

Evidence: [passing RavenPerch log](../../Saved/Logs/Codex_RavenFoxEncounter_Final_20261010.log),
[test source](../../Source/CaptiveSky_2/Agent/Tests/RavenPerchTests.cpp).

## AutoSDK exception correlation

A sandboxed scratch `UnrealEditor-Cmd` launch logged `UBT AutoSDK ReturnCode:
-532462766`, which is the signed form of `0xE0434352`, and also logged
`CreateProc failed: Access is denied`. The same bounded test launched with
approved access to Unreal's user-level build logs returned AutoSDK code `0` and
passed. This directly correlates that controlled startup path with the dialog's
exception code, but it does not prove that every recurring desktop popup has
the same cause or identify the managed exception's internal stack.

Evidence: [sandboxed diagnostic log](../../Saved/Logs/Codex_RavenFoxEncounter_Diagnostics_20261010.log),
[approved bounded run](../../Saved/Logs/Codex_RavenFoxEncounter_Final_20261010.log).
