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


if __name__ == "__main__":
    unittest.main()
