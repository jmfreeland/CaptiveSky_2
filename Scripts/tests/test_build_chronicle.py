import importlib.util
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "Build-Chronicle.py"
SPEC = importlib.util.spec_from_file_location("build_chronicle", SCRIPT)
assert SPEC and SPEC.loader
CHRONICLE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHRONICLE)


class ChronicleRenderTests(unittest.TestCase):
    def test_quiet_activity_is_rendered_as_a_blockquote(self):
        entries = [
            {"type": "decision", "action": "wander", "agent": "Agent_Aster_01", "day": 4, "clock": "08:00"}
        ]
        markdown = CHRONICLE.render(entries, None)
        page = CHRONICLE.to_html(markdown)

        self.assertIn("<blockquote>Otherwise Aster wandered 1", page)
        self.assertNotIn("<p>&gt; Otherwise Aster", page)

    def test_resident_text_is_escaped_in_html(self):
        entries = [
            {
                "type": "decision",
                "action": "speak",
                "agent": "Agent_Aster_01",
                "day": 4,
                "clock": "08:00",
                "text": "<script>not executable</script>",
            }
        ]
        page = CHRONICLE.to_html(CHRONICLE.render(entries, None))

        self.assertIn("&lt;script&gt;not executable&lt;/script&gt;", page)
        self.assertNotIn("<script>not executable</script>", page)

    def test_crab_investigation_and_resident_find_are_in_the_readable_history(self):
        entries = [
            {"type": "wildlife", "agent": "TidepoolCrab", "day": 4, "clock": "11:55",
             "event": "crab_wrack_investigation", "text": "A Tideglass shore crab investigates storm wrack 7."},
            {"type": "wrack_turned", "agent": "Aster", "day": 4, "clock": "12:00",
             "event": "shore_find_discovered", "text": "Turned over kelp and found a small crab's hollow."},
        ]

        markdown = CHRONICLE.render(entries, None)

        self.assertIn("TidepoolCrab A Tideglass shore crab investigates storm wrack 7.", markdown)
        self.assertIn("Aster Turned over kelp and found a small crab's hollow.", markdown)


if __name__ == "__main__":
    unittest.main()
