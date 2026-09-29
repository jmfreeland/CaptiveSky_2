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


if __name__ == "__main__":
    unittest.main()
