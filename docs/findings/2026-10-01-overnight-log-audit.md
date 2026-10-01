# Overnight log audit (2026-10-01)

The recent isolated runtime probes did not show a CaptiveSky process crash, but they did
contain repeated editor-Python errors and one failed Aster probe. Keep those separate
when reading the logs:

- `Codex_InnkeeperWanderProbe_20261001.log` completed the forced innkeeper wander. The
  controller selected a reachable destination with two nearby visible landmarks, then
  completed 1,717 cm of movement in 3.4 simulated seconds. Thinking was disabled and the
  run made zero model requests.
- `Codex_AsterWanderProbe_20261001.log` and `Codex_AsterWanderProbe_TagAster_20261001.log`
  both timed out with `mover=no controller=no target=yes`. The isolated Island world
  spawned its optional innkeeper but no Aster, so these are failed probes, not Aster
  movement failures. Do not count them as validation; use a harness that explicitly
  spawns Aster or a world in which the Aster actor is already present.
- Each of those three launches logged nine `LogPython: Error` tracebacks from UE 5.8's
  Experimental Toolsets startup scripts. The missing symbols include `ToolsetDefinition`,
  `AgentSkill`, `PythonTestRunner`, and a MetaHuman editor subsystem. They occur during
  editor-Python initialization and are independent of the resident movement code. The
  innkeeper run still completed; the two Aster runs reached their expected probe timeout.
  No fatal-error, assertion, ensure, or unhandled-exception marker was found in the three
  probe logs.
- The Aster log also reports that the installed DDC cache was inaccessible and UE fell
  back to memory, plus offline Epic Online Services connection warnings. These indicate
  local cache/network limitations, not gameplay exceptions.

No plugin settings were changed: the project has user-requested Unreal MCP/tooling enabled,
and disabling editor Python or optional Toolsets without checking which integration owns
them could remove capabilities. Revisit after the UE 5.8.3 editor is available and its
startup log can be checked in the user's actual editor setup.

Related prior Aster movement validation is documented in
`docs/findings/2026-09-28-long-runs.md`; these later missing-actor runs do not invalidate
that earlier successful probe, but they do show the current isolated probe setup is not
reproducible for Aster by itself.

## Current editor log check (2026-10-01)

The live editor log `Saved/Logs/CaptiveSky_2.log` (13:12–14:41 UTC) contains 1,201
`libcurl error: 7` request failures. The accompanying connection details name
`www.google.com`, `api.epicgames.dev`, and `datarouter.ol.epicgames.com`; EOS also reports
retrying its SDK platform-config request. The same log has zero `LogCaptiveSky` warning,
error, or fatal entries, zero Python error/traceback entries, and zero fatal, assertion,
ensure, or unhandled-exception markers. This editor session's repeated warnings are
therefore outbound-service connectivity noise, not evidence of CaptiveSky gameplay
exceptions. The external services were not reachable from this machine during the log
interval; no project plugin settings were changed.
