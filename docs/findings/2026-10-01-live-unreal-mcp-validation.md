# Live Unreal MCP validation (2026-10-01)

The running UE 5.8.3 editor (`CaptiveSky_2`, PID 6344) remained responsive while
finishing startup. Once initialized, it listened on `127.0.0.1:8000`. A read-only MCP
handshake completed at `/mcp` using protocol `2025-11-25`; `tools/list` returned the
registry tools, `list_toolsets` exposed 56 discoverable toolsets, and the current level
was `/Game/Maps/Island`. No level or asset was edited or saved during this check.

The old `Codex_IslandTideglassLivelyAsset_20261001.log` run failed with an engine ensure
(`UWorldSubsystem::OnWorldBeginPlay` called twice). Its stack pointed to test line 86 in
the binary used by that run. The current source calls `World->BeginPlay()` once, and the
later `Codex_TideglassLifecycle_Elevated_20261001.log` already showed a passing rerun.
To verify the current live editor state directly, the MCP automation runner discovered
and passed the following focused tests with zero errors and zero warnings:

- `CaptiveSky2.Agent.IslandTideglass`
- `CaptiveSky2.Agent.NightEcology`
- `CaptiveSky2.Agent.RavenPerch`
- `CaptiveSky2.Agent.ResidentApproach`
- `CaptiveSky2.Agent.ResidentWanderPaths`

The current editor log records the successful runs at `13:25:52` and `13:26:19` UTC:
`Saved/Logs/CaptiveSky_2.log`. The initial period with no listener was startup latency;
it was not a persistent MCP failure. The editor’s current viewport was pointed at a
high, rocky ridge rather than the settlement, so it was not a useful beauty capture.
Next visual inspection should use a settlement-level viewpoint before making material
or foliage changes. Keep future play probes bounded and disable autonomous thinking
unless a specific behavior is under test.
