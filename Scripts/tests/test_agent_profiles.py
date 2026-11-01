"""Static resident profiles ship with source; lived state stays local."""

from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
AGENT_ROOT = ROOT / "Agents"
CURRENT_RESIDENTS = {
    "Agent_Aster_01",
    "Agent_Innkeeper_01",
    "Agent_Rat_01",
    "Agent_Raven_01",
}
PRIVATE_STATE_NAMES = {
    "memory.jsonl",
    "places.json",
    "relationships.json",
    "personality_evolution.json",
    "personality_history.jsonl",
}


class AgentProfileTests(unittest.TestCase):
    def test_every_current_resident_has_identity_and_personality(self):
        profiles = {path.name for path in AGENT_ROOT.iterdir() if path.is_dir()}
        self.assertEqual(profiles, CURRENT_RESIDENTS)

        for agent_id in sorted(CURRENT_RESIDENTS):
            with self.subTest(agent_id=agent_id):
                for filename in ("identity.md", "personality.md"):
                    profile = AGENT_ROOT / agent_id / filename
                    self.assertTrue(profile.is_file(), str(profile))
                    self.assertGreater(len(profile.read_text(encoding="utf-8").strip()), 80)

    def test_lived_agent_state_is_not_tracked(self):
        tracked = subprocess.run(
            ["git", "-C", str(ROOT), "ls-files", "--", "Agents"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.splitlines()
        tracked_names = {Path(path).name for path in tracked}
        self.assertFalse(tracked_names & PRIVATE_STATE_NAMES)


if __name__ == "__main__":
    unittest.main()
