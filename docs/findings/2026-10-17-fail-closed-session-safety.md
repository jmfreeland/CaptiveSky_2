# Fail-closed session safety test — 2026-10-17

The current shared working tree contains a fail-closed guard around the
gateway-facing model requests: both production send sites require a successful
request reservation from `UAgentPlaySessionSubsystem`. If that subsystem is
unavailable, the send is rejected instead of bypassing session limits. The
focused test also checks the missing-session case.

The working-tree version was checked in an isolated UE 5.8.3 scratch project
while the user's interactive editor remained open. The editor was left running;
the Island map was not opened or changed. The scratch editor target build
completed successfully (0 actions were needed), and the NullRHI automation test
`CaptiveSky2.Agent.SessionSafety` completed with `Result={Success}` and exit
code 0. Agent thinking was disabled, so this run made no model requests.

Fresh test log:
`Saved/CompileScratch/Codex_VisitorResonance_20261017/Saved/Logs/Codex_AgentSafetyFailClosed_Rerun_20261017.log`

This records verification evidence only. The source changes and test change are
shared, uncommitted working-tree edits and are not included in this finding's
commit; this does not represent the fix as shipped. The source owner should
commit those changes and rerun the focused test after any further edits.
