# Bounded Island Game startup crash (2026-10-29)

## Result

A 60-second, zero-model-request Island capture did not reach the world-ready marker. The launched Unreal process exited almost immediately and produced a crash report. No gameplay, screenshot, or model request occurred. The saved Island and its persistent world data were not edited by this attempt.

The ordinary editor restart succeeded afterward: `CaptiveSky_2` reopened on the project, and `Config/DefaultEngine.ini` still has `bEnableNaniteFoliage=True` and `r.Nanite.Foliage=True`. This proves the setting was loaded by the editor; it does not validate Nanite Foliage in a Game session.

## Bounded launch

The existing `Scripts/Start-Spectator.ps1` harness was used with these safeguards and diagnostics:

- 60-second realtime cap; zero model requests; resident thinking and Python disabled.
- Isolated `Saved/Playtests/Codex_TideglassNaniteRestart_20261029/Data` data root.
- 1280x720 window, Tideglass motion viewpoints, noon start, texture-streaming stats, and a 200-frame CSV profile.
- Startup-timeout supervision. The script detected process exit before the Island world-ready log marker and returned failure rather than treating exit code 0 as proof of a successful load.

The crash report's captured command line includes the project and `/Game/Maps/Island`, followed by `-game -Spectator`, the diagnostic flags, the isolated data root, and the requested log path. This does not look like an accidental launch of another project or map.

## Crash evidence

`Saved/Crashes/_0000/CrashContext.runtime-xml` reports:

- `Unhandled Exception: EXCEPTION_ACCESS_VIOLATION reading address 0x0000000000000000`.
- `SecondsSinceStart=0`; normal crash context fields including the process id, executable, command line, and engine mode are empty.
- Call stack in `UnrealEditor-Core` and `UnrealEditor`; `PCallStackHash` is `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`.
- The signature is unique among the currently retained `Saved/Crashes` contexts.

The accompanying CrashReportClient log reaches `Running engine for game: CaptiveSky_2`, but there is no `Saved/Logs/Codex_TideglassNaniteRestart_20261029.log` and the capture never reports the Island ready. The log also notes missing optional `aqProf`/Vtune DLLs and a failed Unreal Trace Server bootstrap; the available evidence does not establish either as the cause. The failure is too early and too weakly symbolized to attribute to the map, Nanite Foliage, or a particular plugin.

## Next diagnostic

Do not repeat the same full capture blindly. First preserve the current crash artifact. On the next approved runtime check, distinguish engine startup from capture-flag interactions with a minimal, supervised Game launch (same project/map, isolated data root, thinking/Python disabled, zero model requests, and a short cap), while retaining a fresh explicit log path and checking the process exit status plus world-ready marker. Add capture/profile flags only after that baseline reaches the world. If the same null access violation recurs, compare its call-stack hash and collect a symbolized dump before changing project or map state.

The harness, project config, landscape material, and Island package were not edited as part of this diagnosis. The editor was reopened normally and is the appropriate current working session.
