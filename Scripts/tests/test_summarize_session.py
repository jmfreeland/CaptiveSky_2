"""Session reports keep newer resident-agency actions legible."""
from __future__ import annotations

import runpy
import json
from pathlib import Path
import sys
import tempfile
import unittest


sys.dont_write_bytecode = True
SUMMARIZER = Path(__file__).resolve().parents[1] / "Summarize-Session.py"
SESSION = runpy.run_path(str(SUMMARIZER), run_name="session_summarizer")


class SessionSummaryTests(unittest.TestCase):
    def test_current_action_enum_names_requests_and_raven_landing(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_path = Path(temporary_directory) / "session.log"
            log_path.write_text(
                "\n".join(
                    [
                        '[2026.10.31-12.00.00:000][  0]LogAutonomousAgentAI: AsterController_0 decided: "I want a new nest." (Action=8)',
                        '[2026.10.31-12.00.01:000][  1]LogAutonomousAgentAI: AsterController_0 decided: "The perch needs shelter." (Action=9)',
                        '[2026.10.31-12.00.02:000][  2]LogAutonomousAgentAI: AsterController_0 decided: "I will try a new recipe." (Action=10)',
                        '[2026.10.31-12.00.02:500][  2]LogAutonomousAgentAI: Agent_Aster_01 chose speech: "The gusts make the Listening Stones sound almost like a chord."',
                        '[2026.10.31-12.12.00:000][ 20]LogAgentBrain: Optional resident-feedback invitation shown to Agent_Raven_01 after 720 real seconds.',
                        '[2026.10.31-12.12.04:000][ 24]LogAgentBrain: Resident feedback check-in result for Agent_Raven_01: speech "The east fork needs a thicker screen from the wind.".',
                        '[2026.10.31-12.12.10:000][ 30]LogAgentBrain: Optional resident-feedback invitation shown to Agent_Aster_01 after 730 real seconds.',
                        '[2026.10.31-12.12.14:000][ 34]LogAgentBrain: Resident feedback check-in result for Agent_Aster_01: no spoken response; other choices remain available.',
                        '[2026.10.31-12.00.03:000][  3]LogAutonomousAgentAI: RavenAgentAIController_0 decided: "I will settle here." (Action=7)',
                        '[2026.10.31-12.00.04:000][  4]LogAutonomousAgentAI: AsterController_0 outcome: Object request 12345678 was added to the human-review queue; no asset has been made or placed.',
                        '[2026.10.31-12.00.05:000][  5]LogAutonomousAgentAI: AsterController_0 outcome: Upgrade suggestion abcdef01 for west roost was added for human review; nothing in the world changed.',
                    ]
                ),
                encoding="utf-8",
            )
            request_inbox = Path(temporary_directory) / "inbox.jsonl"
            requests = [
                {
                    "id": "12345678-0000-0000-0000-000000000000",
                    "requester": "Aster",
                    "request_type": "new_object",
                    "description": "A moss-lined resting bowl",
                    "status": "pending_review",
                },
                {
                    "id": "abcdef01-0000-0000-0000-000000000000",
                    "requester": "Aster",
                    "request_type": "upgrade",
                    "upgrade_kind": "functionality",
                    "target": "west roost",
                    "description": "Add a wind-sheltered nook",
                    "status": "approved",
                },
                {
                    "id": "99999999-0000-0000-0000-000000000000",
                    "requester": "Raven",
                    "request_type": "upgrade",
                    "description": "An older, unrelated proposal",
                },
            ]
            request_inbox.write_text("\n".join(json.dumps(request) for request in requests), encoding="utf-8")

            report = SESSION["summarize"](log_path, request_inbox)

        self.assertIn("request_object", report)
        self.assertIn("request_upgrade", report)
        self.assertIn("practice_craft", report)
        self.assertIn("Aster: “The gusts make the Listening Stones sound almost like a chord.”", report)
        self.assertIn("## Optional resident feedback", report)
        self.assertIn("Raven (invited after 12 min): speech \"The east fork needs a thicker screen from the wind.\".", report)
        self.assertIn("Aster (invited after 12 min): no spoken response; other choices remain available.", report)
        self.assertIn("| Raven | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 1 |", report)
        self.assertIn("| AsterController_0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 3 |", report)
        self.assertIn("moss-lined resting bowl", report)
        self.assertIn("functionality) for `west roost`", report)
        self.assertNotIn("older, unrelated proposal", report)

    def test_multi_session_log_defaults_to_latest_and_can_select_earlier_run(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_path = Path(temporary_directory) / "multi-session.log"
            log_path.write_text(
                "\n".join(
                    [
                        '[2026.10.31-12.00.00:000][  0]LogAgentSession: Play safety active: 1800 real seconds, at most 64 model requests.',
                        '[2026.10.31-12.00.01:000][  1]LogAutonomousAgentAI: AsterController_0 decided: "The meadow feels open." (Action=2)',
                        '[2026.10.31-12.00.10:000][  2]LogAgentSession: Play ended after 10 real seconds with 1 model requests (0 still in flight).',
                        '[2026.10.31-12.01.00:000][  0]LogAgentSession: Play safety active: 1800 real seconds, at most 64 model requests.',
                        '[2026.10.31-12.01.01:000][  1]LogAutonomousAgentAI: RavenAgentAIController_0 decided: "I will land by the shore." (Action=7)',
                        '[2026.10.31-12.01.10:000][  2]LogAgentSession: Play ended after 10 real seconds with 1 model requests (0 still in flight).',
                    ]
                ),
                encoding="utf-8",
            )

            latest = SESSION["summarize"](log_path)
            earlier = SESSION["summarize"](log_path, session=1)

        self.assertIn("session 2 of 2", latest)
        self.assertIn("| Raven | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 1 |", latest)
        self.assertNotIn("AsterController_0", latest)
        self.assertIn("session 1 of 2", earlier)
        self.assertIn("| AsterController_0 | 0 | 0 | 1 |", earlier)
        self.assertNotIn("Raven |", earlier)

    def test_rat_is_not_misidentified_as_innkeeper_when_hearth_is_mentioned(self) -> None:
        self.assertEqual(
            SESSION["resident_name"]("RatAgentAIController_0", ["I noticed the inn hearth"]),
            "Rat",
        )
        self.assertEqual(SESSION["resident_name"]("Agent_Aster_01", []), "Aster")
        self.assertEqual(SESSION["resident_name"]("Agent_Innkeeper_01", []), "Innkeeper")

    def test_agent_identity_in_new_logs_becomes_the_report_resident_name(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_path = Path(temporary_directory) / "identified-resident.log"
            log_path.write_text(
                '[2026.10.31-12.00.00:000][  0]LogAutonomousAgentAI: Agent_Aster_01 decided: "The coast is interesting." (Action=2)\n',
                encoding="utf-8",
            )

            report = SESSION["summarize"](log_path)

        self.assertIn("| Aster | 0 | 0 | 1 |", report)

    def test_mature_tree_diagnostic_summarizes_local_species_diversity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_path = Path(temporary_directory) / "tree-diagnostic.log"
            log_path.write_text(
                "\n".join(
                    [
                        "[2026.10.31-12.00.00:000][  0]LogAgentSession: Play safety active: 900 real seconds, at most 32 model requests.",
                        "[2026.10.31-12.00.01:000][  1]LogIslandWeather: Mature tree tile [-2,3] (200 m): spruce=4, Norway maple=0, European aspen=2, European beech=0, black alder=1.",
                        "[2026.10.31-12.00.02:000][  2]LogIslandWeather: Mature tree tile [0,3] (200 m): spruce=7, Norway maple=0, European aspen=0, European beech=0, black alder=0.",
                        "[2026.10.31-12.15.00:000][  3]LogAgentSession: Play ended after 900 real seconds with 0 model requests (0 still in flight).",
                    ]
                ),
                encoding="utf-8",
            )

            report = SESSION["summarize"](log_path)

        self.assertIn("## Mature tree distribution (200 m diagnostic)", report)
        self.assertIn("2 occupied tiles; 1 (50%) contain at least two mature-tree species.", report)
        self.assertIn("| Norway maple | 0 | 0 |", report)
        self.assertIn("| European aspen | 1 | 2 |", report)
        self.assertIn("Single-species tile examples: [0,3].", report)


if __name__ == "__main__":
    unittest.main()
