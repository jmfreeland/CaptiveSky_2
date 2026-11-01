"""The spectator launcher must match the game's bounded-play safeguards."""
from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
LAUNCHER = ROOT / "Scripts" / "Start-Spectator.ps1"
GAME_CONFIG = ROOT / "Config" / "DefaultGame.ini"
SESSION_HEADER = ROOT / "Source" / "CaptiveSky_2" / "Agent" / "AgentPlaySessionSubsystem.h"


class StartSpectatorCapTests(unittest.TestCase):
    def test_launcher_default_and_hard_request_caps_stay_aligned(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")
        config = GAME_CONFIG.read_text(encoding="utf-8")
        session_header = SESSION_HEADER.read_text(encoding="utf-8")

        launcher_parameter = re.search(
            r"\[ValidateRange\(0,\s*(\d+)\)\]\[Nullable\[int\]\]\$MaxModelRequests",
            launcher,
        )
        launcher_default = re.search(r"minutes and (\d+) model requests", launcher)
        configured_default = re.search(r"(?m)^MaxModelRequests=(\d+)\s*$", config)
        hard_limit = re.search(r"MaxModelRequestsHardLimit\s*=\s*(\d+)", session_header)

        self.assertIsNotNone(launcher_parameter)
        self.assertIsNotNone(launcher_default)
        self.assertIsNotNone(configured_default)
        self.assertIsNotNone(hard_limit)

        limits = {
            "launcher parameter maximum": int(launcher_parameter.group(1)),
            "launcher documented default": int(launcher_default.group(1)),
            "project configured default": int(configured_default.group(1)),
            "runtime hard limit": int(hard_limit.group(1)),
        }
        self.assertEqual(limits, {name: 64 for name in limits})

    def test_dragonfly_visit_logging_requires_isolated_bounded_diagnostics(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")
        ecology_source = ROOT / "Source" / "CaptiveSky_2" / "Agent" / "IslandTideglassDragonfly.cpp"
        ecology = ecology_source.read_text(encoding="utf-8")

        self.assertIn("[switch]$LogDragonflyFlowerVisits", launcher)
        self.assertIn("-LogDragonflyFlowerVisits requires an isolated -DataRoot", launcher)
        self.assertIn("-LogDragonflyFlowerVisits requires -DisableAgentThinking", launcher)
        self.assertIn(
            "-LogDragonflyFlowerVisits requires explicit -StartupTimeoutSeconds, -MaxRealtimeSeconds, and -MaxModelRequests caps",
            launcher,
        )
        self.assertIn('"CaptiveSky.Island.DragonflyFlowerTelemetry 1"', launcher)
        self.assertIn('TEXT("CaptiveSky.Island.DragonflyFlowerTelemetry")', ecology)

    def test_null_rhi_requires_isolated_bounded_zero_thinking_run(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$NullRHI", launcher)
        self.assertIn("-NullRHI requires an isolated -DataRoot", launcher)
        self.assertIn("-NullRHI requires -DisableAgentThinking unless the explicit bounded -NullRHICharacterFeedback mode is used", launcher)
        self.assertIn(
            "-NullRHI requires explicit -StartupTimeoutSeconds, -MaxRealtimeSeconds, and -MaxModelRequests caps",
            launcher,
        )
        self.assertIn('if ($NullRHI) { $arguments += "-NullRHI" }', launcher)
        self.assertIn("-NullRHI cannot be combined with -Shots", launcher)
        self.assertIn('$windowStyle = if ($NullRHI) { "Hidden" } else { "Normal" }', launcher)

    def test_null_rhi_character_feedback_is_explicit_and_tightly_bounded(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$NullRHICharacterFeedback", launcher)
        self.assertIn("-NullRHICharacterFeedback requires -NullRHI", launcher)
        self.assertIn("-NullRHICharacterFeedback requires an isolated -DataRoot", launcher)
        self.assertIn("-NullRHICharacterFeedback requires -LogSituations", launcher)
        self.assertIn("-NullRHICharacterFeedback requires -DisablePython", launcher)
        self.assertIn("-NullRHICharacterFeedback is capped at 900 real-time seconds", launcher)
        self.assertIn("-NullRHICharacterFeedback requires a model-request cap from 1 through 64", launcher)
        self.assertIn("this is the existing hard ceiling", launcher)
        self.assertIn("-NullRHICharacterFeedback requires resident thinking to remain enabled", launcher)

    def test_commandlet_executable_is_confined_to_hidden_bounded_null_rhi(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$UseCommandletExecutable", launcher)
        self.assertIn("-UseCommandletExecutable requires -NullRHI", launcher)
        self.assertIn("never selects the commandlet executable for rendered play", launcher)
        self.assertIn("-UseCommandletExecutable requires a bounded session", launcher)
        self.assertIn('if ($UseCommandletExecutable) { "UnrealEditor-Cmd.exe" } else { "UnrealEditor.exe" }', launcher)
        self.assertIn("-UseCommandletExecutable -NullRHI -DisableAgentThinking", launcher)

    def test_trace_server_disable_is_limited_to_null_rhi(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$DisableTraceServer", launcher)
        self.assertIn("-DisableTraceServer is restricted to hidden, non-rendered -NullRHI diagnostics", launcher)
        self.assertIn('if ($DisableTraceServer) { $arguments += "-notraceserver" }', launcher)

    def test_gpu_headroom_gate_is_opt_in_and_runs_before_project_launch(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$RequireGpuHeadroom", launcher)
        self.assertIn("$MinimumFreeGpuMemoryMiB = 4096", launcher)
        self.assertIn("Get-Command nvidia-smi", launcher)
        self.assertIn("--query-gpu=memory.used,memory.total", launcher)
        self.assertIn("Assert-GpuMemoryHeadroom", launcher)
        self.assertIn("-RequireGpuHeadroom is only applicable to rendered sessions", launcher)
        self.assertLess(launcher.index("if ($RequireGpuHeadroom) {"), launcher.index("$projectCandidate ="))

        module = (ROOT / "Scripts" / "GpuMemoryBudget.psm1").read_text(encoding="utf-8")
        self.assertIn("No rendered session was started", module)
        self.assertIn("$freeMiB -lt $MinimumFreeMiB", module)

    def test_gpu_headroom_monitor_is_opt_in_bounded_and_only_stops_its_game(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$MonitorGpuHeadroom", launcher)
        self.assertIn("-MonitorGpuHeadroom requires -RequireGpuHeadroom", launcher)
        self.assertIn("-MonitorGpuHeadroom requires a bounded session", launcher)
        self.assertIn("-MonitorGpuHeadroom requires an isolated -DataRoot", launcher)
        self.assertIn("-MonitorGpuHeadroom requires explicit -StartupTimeoutSeconds, -MaxRealtimeSeconds, and -MaxModelRequests caps", launcher)
        self.assertIn("WaitForExit($GpuMemoryMonitorIntervalSeconds * 1000)", launcher)
        self.assertIn("Get-GpuHeadroomMonitorDecision", launcher)
        self.assertIn("$Game.Kill($true)", launcher)
        self.assertIn("Only the Game process tree launched by this script will be terminated", launcher)
        self.assertIn("-MonitorGpuHeadroom to a bounded, isolated rendered run", launcher)

    def test_system_memory_gate_checks_ram_before_project_launch(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$RequireSystemMemoryHeadroom", launcher)
        self.assertIn("$MinimumFreeSystemMemoryMiB = 4096", launcher)
        self.assertIn("Get-CimInstance -ClassName Win32_OperatingSystem", launcher)
        self.assertIn("FreePhysicalMemory", launcher)
        self.assertIn("Assert-SystemMemoryHeadroom", launcher)
        self.assertIn("-RequireSystemMemoryHeadroom is only applicable to rendered sessions", launcher)
        self.assertLess(launcher.index("if ($RequireSystemMemoryHeadroom) {"), launcher.index("$projectCandidate ="))

        module = (ROOT / "Scripts" / "SystemMemoryBudget.psm1").read_text(encoding="utf-8")
        self.assertIn("No rendered session was started", module)
        self.assertIn("$FreeMiB -lt $MinimumFreeMiB", module)

    def test_system_memory_monitor_is_bounded_and_process_scoped(self) -> None:
        launcher = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("[switch]$MonitorSystemMemoryHeadroom", launcher)
        self.assertIn("-MonitorSystemMemoryHeadroom requires -RequireSystemMemoryHeadroom", launcher)
        self.assertIn("-MonitorSystemMemoryHeadroom requires a bounded session", launcher)
        self.assertIn("-MonitorSystemMemoryHeadroom requires an isolated -DataRoot", launcher)
        self.assertIn("-MonitorSystemMemoryHeadroom requires explicit -StartupTimeoutSeconds, -MaxRealtimeSeconds, and -MaxModelRequests caps", launcher)
        self.assertIn("Get-SystemMemoryHeadroomMonitorDecision", launcher)
        self.assertIn("$Game.Kill($true)", launcher)
        self.assertIn("Only the Game process tree launched by this script will be terminated", launcher)

        module = (ROOT / "Scripts" / "SystemMemoryBudget.psm1").read_text(encoding="utf-8")
        self.assertIn("ShouldStop = ($nextLowSamples -ge $RequiredConsecutiveLowSamples)", module)


if __name__ == "__main__":
    unittest.main()
