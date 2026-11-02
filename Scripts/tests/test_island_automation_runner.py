"""The focused Unreal automation runner must remain isolated and bounded."""
from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "Scripts" / "Run-IslandAutomation.ps1"


class IslandAutomationRunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.runner = RUNNER.read_text(encoding="utf-8")

    def test_uses_editor_cmd_without_game_mode_and_null_rhi(self) -> None:
        self.assertIn("UnrealEditor-Cmd.exe", self.runner)
        self.assertIn("'-NullRHI'", self.runner)
        self.assertNotIn("'-game'", self.runner)
        self.assertIn("-ExecCmds=Automation RunTests", self.runner)
        self.assertIn("-TestExit=Automation Test Queue Empty", self.runner)

    def test_test_filter_and_data_root_are_restricted(self) -> None:
        self.assertIn("[ValidatePattern('^[A-Za-z0-9_.+]+$')]", self.runner)
        self.assertIn("Join-Path $projectRoot 'Saved\\Playtests'", self.runner)
        self.assertIn("Automation_$runId", self.runner)
        self.assertIn("Refusing to reuse an existing automation data root", self.runner)
        self.assertIn("'-CaptiveSkyDisableAgentThinking'", self.runner)
        self.assertIn("'-CaptiveSkyMaxModelRequests=0'", self.runner)

    def test_memory_preflight_and_process_timeout_guard_launch(self) -> None:
        self.assertIn("MinimumFreeSystemMemoryMiB = 4096", self.runner)
        self.assertIn("Assert-SystemMemoryHeadroom", self.runner)
        self.assertIn("Get-SystemMemoryHeadroomMonitorDecision", self.runner)
        self.assertIn("$process.Kill($true)", self.runner)
        self.assertIn("TimeoutSeconds = 240", self.runner)
        self.assertLess(self.runner.index("Assert-SystemMemoryHeadroom"), self.runner.index("Start-Process"))
        self.assertLess(self.runner.index("if ($DryRun)"), self.runner.index("Start-Process"))

    def test_only_named_automation_success_counts_as_behavior_evidence(self) -> None:
        self.assertIn("Test Completed\\. Result=\\{Success\\}", self.runner)
        self.assertIn("foreach ($testPath in ($Test -split '\\+'))", self.runner)
        self.assertIn("CrashContext.runtime-xml", self.runner)
        self.assertIn("HtmlDecode", self.runner)
        self.assertIn("wrote no requested log", self.runner)


if __name__ == "__main__":
    unittest.main()
