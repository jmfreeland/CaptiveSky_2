# UE 5.8.3 headless resident validation (2026-10-08)

The current shared source tree built successfully for `CaptiveSky_2Editor` with
UE 5.8.3 (`PatchVersion: 3`, changelist 58210709). The editor GUI and Unreal
MCP endpoint were unavailable, so validation used `UnrealEditor-Cmd` with
`-NullRHI -NoSound -DisablePython`, agent thinking disabled, a zero model
request cap, and distinct `Saved/Playtests/` data roots.

Fresh automation runs passed:

- `CaptiveSky2.Agent.WoodlandDeer` — quiet resident-presence response.
- `CaptiveSky2.Agent.RavenPerch` — collision-aware movement between roosts.
- `CaptiveSky2.Agent.IslandArrangement` and
  `CaptiveSky2.Agent.IslandArrangementInspection` — persistent arrangement
  state, public lineage and inspection/known-influence boundaries.
- `CaptiveSky2.Agent.NightEcology` — bounded night insects, minnow and crab
  responses, weather effects, and wildlife reactions covered by the shared
  ecology regression.

Logs: [`Codex_HeadlessAgency_20261008.log`](../../Saved/Logs/Codex_HeadlessAgency_20261008.log),
[`Codex_HeadlessRavenPerch_20261008.log`](../../Saved/Logs/Codex_HeadlessRavenPerch_20261008.log),
[`Codex_HeadlessArrangement_20261008.log`](../../Saved/Logs/Codex_HeadlessArrangement_20261008.log),
and [`Codex_HeadlessNightEcology_20261008.log`](../../Saved/Logs/Codex_HeadlessNightEcology_20261008.log).

These checks validate compilation and deterministic behavior only. They do not
prove animation readability, voluntary resident choices, rendered appearance,
or ordinary PIE performance. No project map or authored asset was changed;
visual review and bounded in-world validation remain next once the editor/MCP
service is available.

## Additional agency regressions (2026-10-08)

While the interactive editor remained blocked in startup SDK verification, four
additional UE 5.8.3 `UnrealEditor-Cmd` automation runs passed in separate
processes:

- `CaptiveSky2.Agent.SocialPacing` — mutual reservations, the four-line
  conversation bound, and the minimum five-minute real-time pair cooldown.
- `CaptiveSky2.Agent.ResidentApproach` — ground/air stand-off geometry for
  residents approaching one another.
- `CaptiveSky2.Agent.BlockedGroundMoveApproach` — a blocked resident can still
  inspect a nearby clearly visible landmark, while distant or occluded cases
  remain movement failures.
- `CaptiveSky2.Agent.ResidentWanderPaths` — reachable-path selection and
  rejection of partial or degenerate routes.

Each run used `-NullRHI -NoSound -DisablePython
-CaptiveSkyDisableAgentThinking -CaptiveSkyMaxModelRequests=0`, a memory DDC,
and its own data root. Logs: [`Codex_SocialPacing_20261008.log`](../../Saved/Logs/Codex_SocialPacing_20261008.log),
[`Codex_BlockedApproach_20261008.log`](../../Saved/Logs/Codex_BlockedApproach_20261008.log),
and [`Codex_ResidentWander_20261008.log`](../../Saved/Logs/Codex_ResidentWander_20261008.log).
`ResidentApproach` is in [`Codex_HeadlessResidentApproach_20261008.log`](../../Saved/Logs/Codex_HeadlessResidentApproach_20261008.log);
that log also records the first combined attempt, which only ran its first test.

`-ExecCmds` splits semicolon-separated console commands before automation
finishes, so repeating `Automation RunTests ...` commands in one string emits
unknown-command warnings for later entries. A zero process exit code alone is
not sufficient evidence: verify each requested test has its own
`Test Completed. Result={Success}` record, or run it in a separate process as
above. These regressions cover deterministic mechanics only; they do not prove
that Aster or the raven will voluntarily approach or speak during ordinary
play.
