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

Logs: [`Codex_HeadlessAgency_20261008.log`](../../Saved/Logs/Codex_HeadlessAgency_20261008.log),
[`Codex_HeadlessRavenPerch_20261008.log`](../../Saved/Logs/Codex_HeadlessRavenPerch_20261008.log),
and [`Codex_HeadlessArrangement_20261008.log`](../../Saved/Logs/Codex_HeadlessArrangement_20261008.log).

These checks validate compilation and deterministic behavior only. They do not
prove animation readability, voluntary resident choices, rendered appearance,
or ordinary PIE performance. No project map or authored asset was changed;
visual review and bounded in-world validation remain next once the editor/MCP
service is available.
