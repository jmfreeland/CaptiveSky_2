# Project bootstrap A/B with the MCP plugin — 2026-10-17

The project-opening NullRHI automation attempt previously crashed before the
project log or test run, with
`PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The project's
`.uproject` enables `ModelContextProtocol` alongside the separate engine plugin
`MCPClientToolset`.

In the ignored `Codex_VisitorResonance_20261017` scratch project, a UE 5.8.3
editor target linked successfully. With `ModelContextProtocol` disabled,
`CaptiveSky2.Agent.FireflyCorridorPlacement` and the corrected
`CaptiveSky2.Agent.NightEcology` passed under NullRHI, thinking disabled, zero
model requests, and `-NoWorldState`.

A paired scratch startup with `ModelContextProtocol` enabled also loaded the
plugin's engine, editor, and engine-extension modules, mounted the separate
`MCPClientToolset`, registered tool-search meta-tools, and passed
`FireflyCorridorPlacement`. A further enabled-plugin run passed the full
`NightEcology` suite with exit code 0. Thus the project MCP plugin is not by
itself sufficient to reproduce the earlier crash. The prior Core-only
null-access violation remains unexplained; the successful runs followed a full
scratch link and warmed project-local cache, so the cause could be first-start
cache/build state or an intermittent startup path. No permanent plugin toggle
is justified.

The main `.uproject`, enabled MCP plugin, and saved map were not modified. PID
22688 was observed unresponsive after an earlier scratch build and later absent
from the process list; this work did not terminate it. Keep the project's MCP
plugin enabled for normal editor use. The crash is not currently reproducible
in the linked, warmed scratch project; capture the next occurrence before
changing plugin settings. The remaining gap is real-RHI night/play review: a
NullRHI automation pass verifies deterministic behavior, not visual readability
or gameplay feel.
