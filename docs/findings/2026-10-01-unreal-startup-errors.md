# Unreal startup error triage (2026-10-01)

## Scope

Checked the UE 5.8.3 log for the bounded `Codex_NightIsland_20261001` spectator
session, plus its matching editor backup log. This was a 60-second, no-thinking
run; the session ended itself after 60.2 real seconds with zero model requests.

## Findings

- Nine Python `AttributeError` traces appeared while Unreal ran optional
  experimental Toolsets startup scripts. They come from eight plugins:
  `AIModuleToolset`, `AnimationAssistantToolset`, `ConversationToolset`,
  `EditorToolset`, `MetaHumanGenerator`, `NiagaraToolsets`,
  `SequencerAnimMixerToolset`, and `StateTreeToolset`.
- The missing Python API symbols are `unreal.ToolsetDefinition`,
  `unreal.AgentSkill`, `unreal.MetaHumanCharacterEditorSubsystem`, and
  `unreal.PythonTestRunner`. The traces point into the installed UE engine's
  `Engine/Plugins/Experimental/Toolsets/.../Content/Python/init_unreal.py` or
  imported modules, not CaptiveSky gameplay code.
- The project's `CaptiveSky_2.uproject` explicitly enables `AllToolsets` and
  many individual Toolsets. The `AllToolsets` descriptor itself describes an
  aggregator over the experimental suite. The UE `ModelContextProtocol`
  descriptor depends on `ToolsetRegistry`, and `MCPClientToolset` also depends
  on that registry. Broadly disabling the stack without a separate MCP
  connectivity check could remove editor capabilities the project intentionally
  enabled.
- In this session's actual play interval there were no `Fatal error`,
  `Unhandled Exception`, C++ `Exception`, failed assertion, or failed `ensure`
  lines. One `LogCrowdFollowing` warning says no `RecastNavMesh` was available
  while creating a `UCrowdManager`; it did not prevent the capped run from
  ending normally.
- A separate `LogTemp: Error test: UE::UnifiedErrorTest...` line belongs to
  Unreal's error-reporting test output, not a CaptiveSky failure.

## Decision

Do not disable `ModelContextProtocol`, `ToolsetRegistry`, `MCPClientToolset`,
or the experimental toolset bundle as part of this triage. A safer first step
is to use Unreal's documented `-DisablePython` launch flag for a game-only
spectator run; the [UE 5.8 command-line reference](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-command-line-arguments-reference?lang=en-US)
lists this flag. This leaves the project's plugin configuration untouched.

That test was run with `-NullRHI`, `-DDC-ForceMemoryCache`,
`-CaptiveSkyDisableAgentThinking`, a unique `-CaptiveSkyDataRoot`, a 60-second
real-time cap, and a one-request cap. It ended normally after 60.1 seconds with
zero model requests. The log reports Python disabled and contains no Python
Toolset tracebacks. The C++ `ModelContextProtocol` and
`ModelContextProtocolEngine` modules still loaded. Because this was game mode,
the editor-only `MCPClientToolset` was not exercised; this does not prove the
editor MCP client works with Python disabled.

`Scripts/Start-Spectator.ps1` now exposes this as opt-in `-DisablePython` for
game-only runs. It is deliberately not enabled by default, and the project's
plugin configuration remains unchanged. Treat the repeated Python traces as
engine/plugin-startup noise, not as proof that residents or world interactions
crashed.

The one crowd-following warning is worth revisiting only if a play session
shows actual navigation failure. It is currently an isolated warning, not
evidence of a failed gameplay behavior.

## 2026-10-13 — Project-open access violation remains unsymbolized

Several later project launches failed before the Island or requested automation
test began. The latest bounded `-NullRHI` attempt targeted
`CaptiveSky2.Agent.NightEcology`; its crash-side log records the command line,
but no normal project log or test result was produced. `CrashContext.runtime-xml`
reports `EXCEPTION_ACCESS_VIOLATION` reading `0x0000000000000000` on the game
thread, with `PCallStackHash`
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The included
`UEMinidump.dmp` is 321,103 bytes. Its recorded stack names only
`UnrealEditor-Core` and `UnrealEditor-Cmd` offsets; there are no function
symbols, so the faulting initializer/module is not identified. This is distinct
from the earlier optional Toolsets Python tracebacks above. Evidence is in
[`Codex_MinnowNightRest_20261013.log`](../../Saved/Crashes/_0000/Codex_MinnowNightRest_20261013.log),
[`CrashContext.runtime-xml`](../../Saved/Crashes/_0000/CrashContext.runtime-xml),
and [`UEMinidump.dmp`](../../Saved/Crashes/_0000/UEMinidump.dmp).

The installed engine location is `D:/Games/Epic/UE_5.8`. The matching
`UnrealEditor-Core.pdb` and `UnrealEditor-Cmd.pdb` are absent from its Win64
binary directory, and the standard Windows Kits x64 `cdb.exe`/`windbg.exe`
paths are absent as well. Engine-only `UnrealEditor-Cmd.exe -version` has
previously exited successfully; that does not isolate the project-opening
failure further.

**Decision:** do not repeat the same project-open launch yet. No evidence here
identifies a plugin, project module, or asset as the cause, so do not disable
plugins or edit project startup configuration speculatively. The next
diagnostic step is to make a matching UE 5.8.3 debugger and symbols available,
then symbolize this existing dump before another project launch. Once the
faulting frame is known, make one targeted environment/configuration change and
retry the zero-request automation. Until then, in-engine visual and behavioral
validation remains unavailable; this finding does not validate Island content.

### Follow-up bounded Game startup — 2026-10-13 12:12 UTC

A later 90-second spectator launch of `/Game/Maps/Island` also exited before
the world became ready. It was configured for zero model requests and would
have written screenshots and a 300-frame profile, but neither was produced.
Its crash log contains engine startup metadata and the command line only; no
normal project log or world-ready marker appears. The crash context again says
`EXCEPTION_ACCESS_VIOLATION` reading the null address, but its
`PCallStackHash` is `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`, not the earlier
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`. Its unsymbolized stack is mostly
`UnrealEditor-Core` offsets, so the shared exception type alone does not prove
the same fault. The dump is 316,090 bytes.

Evidence: [`SpectatorStartup_20261013_131240_538.log`](../../Saved/Crashes/_0000/SpectatorStartup_20261013_131240_538.log)
and the same crash-context/minidump files linked above. The baseline output
directory exists, but no capture or profile is available; the proposed
ground-cover-density comparison was not run.

**Decision:** keep the no-retry gate. Symbolize both existing dumps with
matching UE 5.8.3 symbols (or obtain equivalent diagnostics) before another
project-open attempt; do not infer a shared root cause from these two hashes.

### Static plugin inventory and build check — 2026-10-13

A read-only descriptor inventory found 39 enabled plugin names in
`CaptiveSky_2.uproject`; every name resolved to a `.uplugin` descriptor under
the installed UE 5.8 `Engine/Plugins`, and no project-local `Plugins/*.uplugin`
was present. This rules out a missing plugin descriptor, not a binary,
dependency, or plugin startup fault. A build-only `CaptiveSky_2Editor` compile
also succeeded (five actions), with warnings that `MetaHumanGenerator` and the
project depend on `StructUtils`, deprecated since UE 5.5. The warning is not
evidence that it caused the null access violation. No plugin was disabled and
no project-open attempt was made; the matching-symbols gate remains.
