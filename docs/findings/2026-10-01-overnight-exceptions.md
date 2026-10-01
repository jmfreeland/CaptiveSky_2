# Overnight exception triage (2026-10-01)

The noisy exceptions in the recent editor-backed Game runs are Python startup failures from UE 5.8's Experimental Toolsets plugins, not exceptions thrown by Aster, Raven, or the ecology systems. `CaptiveSky_2.uproject` enables `AllToolsets` and its individual toolsets. The nine tracebacks in each of `Codex_WorldEcologyVisual_20261001_corrected.log`, `Codex_ScreenshotIsolation_20261001_retry.log`, and `InnMovementProbe_Wander.log` originate under `Engine/Plugins/Experimental/Toolsets`; recurring failures include missing `unreal.ToolsetDefinition`, `unreal.AgentSkill`, and `unreal.MetaHumanCharacterEditorSubsystem` symbols. Each affected launch therefore contains 72 `LogPython: Error` lines, but zero fatal/crash markers. The failures occur at plugin startup, before the isolated movement probe begins.

The focused `CaptiveSky2.Agent.NightEcology` automation run on the current UE 5.8.3 build completed successfully on 2026-10-01. Its log contains zero Python error lines, DDC errors, or fatal/crash markers. The same test had a genuine historical access violation on 2026-09-27 inside Unreal's UMG code. The stack pointed to `FIslandNightEcologyTest::RunTest` at then-current line 467; that source snapshot maps the line to `UMaterialInterface::GetAllScalarParameterInfo` while introspecting the authored volumetric-cloud material. That is the best-supported trigger, though the incomplete engine callstack does not prove the deeper cause. The crash site is no longer present in the current test, a subsequent run later that evening passed, and today's focused rerun also passed. Treat that crash as not currently reproducible, not as a harmless Python traceback.

No project plugin settings were changed during this triage. The experimental toolsets may be part of the user's Unreal/MCP editor workflow, so disabling them merely to quiet headless logs would risk removing useful editor capability. If the startup noise needs to be removed, first test a probe-only launch configuration that retains the required MCP integration, then verify editor-side MCP still works before changing project defaults.

## Evidence

- `Saved/Logs/NightEcology_Current_20261001.log` — `CaptiveSky2.Agent.NightEcology` success; no Python, DDC, or fatal errors.
- `Saved/Logs/Codex_WorldEcologyVisual_20261001_corrected.log` — nine Experimental Toolsets Python tracebacks; no fatal errors.
- `Saved/Logs/Codex_ScreenshotIsolation_20261001_retry.log` — same nine plugin-startup tracebacks; no fatal errors.
- `Saved/Logs/InnMovementProbe_Wander.log` — same nine plugin-startup tracebacks; raven flight-wander completed, zero model requests, no fatal errors.
- `Saved/Logs/Saved/Logs/CaptiveSky_Automation-backup-2026.09.27-18.15.44.log` — historical UMG access violation during Night Ecology automation.
- `Saved/Logs/Saved/Logs/CaptiveSky_Automation.log` — subsequent Night Ecology automation success on 2026-09-27.
