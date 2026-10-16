# Rat-hollow candidate review did not reach the Island — 2026-10-16

## Attempt

Site A (`-105200, 104200, 2334` cm) was selected for a disposable real-RHI
review using the existing Rat-eye and human-approach viewpoints in
`Config/CodexRatHollowSiteTwoCandidateReview.json`. A one-record Rat-home fixture
was placed under the isolated data root at
`Saved/Playtests/Codex_RatHollowSiteReview_20261016_SiteA/World`; it was not
added to the Rat's canonical `places.json`. Resident thinking was disabled,
the model-request cap was zero, and the Game runtime cap was 120 seconds.

The corrected Unreal process passed the Win64 SDK validation (`UBT AutoSDK
ReturnCode: 0`) and initialized through Slate, but its log stopped before any
Island/map/world-ready marker. It remained alive and responsive for more than
nine minutes without log progress; no screenshot appeared. Only that exact
disposable process was stopped. The separate user editor process was left
untouched. No saved Island or canonical resident/world-state file was changed.

Evidence: [run log](../../Saved/Logs/Codex_RatHollowSiteReview_20261016_SiteA_Waited.log).
The earlier monitor-wrapper attempt omitted the project argument and is not
counted as project-startup evidence.

## Decision

Neither Site A nor Site B is approved. The review produced no visual evidence,
and the earlier bounded scan found no candidate meeting its spruce-crown gate.
Keep `Agents/Agent_Rat_01/places.json` empty. Resume the paired Rat-eye,
human-approach, and capsule/navigation review only when the Island can load in
a responsive test/editor session; then test one candidate at a time with an
isolated temporary wildlife-home record.

Separately, the current scratch build was up to date and
`CaptiveSky2.Agent.SessionSafety` passed under NullRHI with Python disabled and
zero model requests ([log](../../Saved/Logs/Codex_SessionSafetyFailClosed_20261016.log)).
That validates the request-safety helper, not fox/firefly screenshot behavior.
