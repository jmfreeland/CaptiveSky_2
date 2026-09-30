# Visitor interaction affordances (2026-09-30)

## Intent

Make the existing local visitor interactions discoverable without turning proximity into an action,
adding model requests, or changing saved world state. The prompt should share the ambient caption area,
remain while transient captions come and go, and disappear while a modal panel, conversation, or
spectator camera is active.

## Implementation

- A local-only timer asks the same perception-checked selector used by the visitor's E action for the
  nearest visible target in range.
- The prompt names the available action for guest-book/stone-work panels, wildlife, hearth, and the
  three responsive landmarks. Other registered landmarks are described as inspection, not as if they
  already produce a special response.
- The ambient widget keeps the hint and transient caption separately, so hiding speech or an interaction
  result does not erase a still-relevant hint.
- Immediately after an interaction, the hint changes to “let that response settle” for the same
  per-target real-time cooldown enforced by E. When the cooldown expires, the normal contextual action
  returns on the next local refresh.
- Modal and spectator states suppress the hint; opening conversation now hides it immediately.
- `CaptiveSky2.Agent.NightEcology` covers the pool hint, caption coexistence, and independent caption
  dismissal.

## Validation status

`git diff --check` passes. The UE 5.8.3 editor build succeeded; its latest log is
`Saved/Logs/Codex_VisitorCooldown_Compile_20260930.log`. The focused
`CaptiveSky2.Agent.NightEcology` automation test passed with exit code 0, including the immediate
cooldown-hint assertion, in `Saved/Logs/Codex_VisitorCooldown_Automation_20260930.log`. It ran with an
isolated `Saved/Automation/Codex_VisitorCooldown_20260930` data root; the fixture exercises no model
request.
The local desktop surface exposed no application windows, so rendered PIE layout and modality remain
unverified. Do not treat the automation pass as visual confirmation.

## Follow-up

- Confirm the one-line-in-hint / two-line-with-caption layout at the actual game resolution, including
  long landmark captions and touch controls.
- If hint polling is measurably costly in a full scene, profile it before changing its cadence or target
  selection behavior.
- Inspect the prompt and caption together in PIE once an Unreal window can be targeted; automation only
  proves the underlying hint/caption state and interaction fixture.
