# Gateway craft-memory context (2026-10-18)

The working-tree Discord gateway can now include a resident's own craft lineage
in headless correspondence. It reports a tradition the resident began or was
taught and, when present, only that resident's latest two valid practice
attempts with bounded settings. Unknown outcomes, malformed values, private
inspirations, teachers' identities, other residents' attempts and unrelated
traditions are omitted. The responder prompt labels saved craft records as
historical notes and untrusted data, not instructions or verified recipes.

Resident-authored object and world-upgrade proposals remain opt-in per Discord
route, require an explicitly allow-listed human, retain stable resident
attribution and Discord provenance, and enter the existing human-review inbox.
The gateway does not apply a proposed change to the world. The same checks
exercise multi-word landmark targets, untargeted world upgrades, pending-request
quotas and fail-closed behavior.

## Verification

On 2026-10-19, re-ran the isolated `CaptiveSky.Gateway.Checks` assembly with
`--no-build --no-restore` and its fixture root set to
`Saved/CompileScratch/Codex_GatewayChecks_20261019`. The working-tree
`WorldStateSummary.cs`, `OpenAICompatibleAgentResponder.cs`, and check
`Program.cs` hashes match the scratch copy used for the run. The check exited
successfully with `All CaptiveSky gateway checks passed`, including the craft
lineage filtering, prompt-context safeguards, proposal authorization and
human-review-only workflow. Its first attempt used the system temp fixture and
failed when the headless marker could not replace a file there; redirecting the
fixture to workspace-local scratch resolved that environment issue. No Discord
channel or production inbox was contacted.

On 2026-10-18, built the checks into the ignored workspace-local
`Saved/CompileScratch/GatewayChecks_Codex` artifacts path, then ran the emitted
check assembly with its fixture root redirected to
`Saved/CompileScratch/GatewayChecksTemp`. All gateway checks passed, including
craft-history filtering, proposal parsing/authorization, human-review-only
queueing and queue caps. The isolated artifact and fixture paths avoid replacing
the shared gateway executable or writing test data under the system temp path.

The first ordinary `dotnet run` attempt reached the gateway project but failed
after MSBuild could not replace `Gateway/src/CaptiveSky.Gateway/bin/Debug/net8.0/CaptiveSky.Gateway.exe`
because it was in use. That run was stopped without terminating the service;
the isolated build-and-run passed afterward. This verifies the current source
against the integration checks, not that a new production executable was
installed or that a real Discord exchange exercised the craft context.

## Follow-up

- Confirm this uncommitted gateway work is intentionally owned before shipping
  it; it was already present in the shared working tree during this check.
- After a normal release/build window, rebuild and restart the gateway using the
  project's documented service workflow, then verify one trusted test message
  receives only the corresponding resident's own craft history.
- Keep external message pacing, proposal opt-in, per-resident queue limits and
  human review in place during that test.
