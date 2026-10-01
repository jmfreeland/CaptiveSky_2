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
or the experimental toolset bundle as part of this triage. First verify which
UE MCP surfaces the user relies on, then test a temporary, reversible launch
with the unnecessary toolset plugins disabled while retaining the MCP server
and client. Treat the repeated Python traces as engine/plugin-startup noise for
now, not as proof that residents or world interactions crashed.

The one crowd-following warning is worth revisiting only if a play session
shows actual navigation failure. It is currently an isolated warning, not
evidence of a failed gameplay behavior.
