import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "requests.py"
SPEC = importlib.util.spec_from_file_location("asset_requests", SCRIPT_PATH)
asset_requests = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(asset_requests)


class RequestInboxTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.inbox = Path(self.directory.name) / "inbox.jsonl"

    def tearDown(self):
        self.directory.cleanup()

    def test_reads_jsonl_and_pretty_printed_legacy_records(self):
        self.inbox.write_text(
            '{"id":"new","requester":"Aster","description":"A nest","status":"pending_review"}\n'
            '{\n  "id": "legacy",\n  "description": "A sign",\n  "pipeline": "ComfyBlender"\n}\n',
            encoding="utf-8",
        )
        records = asset_requests.read_records(self.inbox)
        self.assertEqual([record["id"] for record in records], ["new", "legacy"])
        self.assertEqual(records[1]["status"], "pending_review")

    def test_status_update_preserves_fields_and_only_changes_status(self):
        original = {
            "id": "req-1", "requester": "Raven", "description": "Add a sheltered perch",
            "request_type": "upgrade", "target": "WindArch", "upgrade_kind": "functionality",
            "pipeline": "human_review", "status": "pending_review",
        }
        self.inbox.write_text(json.dumps(original) + "\n", encoding="utf-8")
        self.assertEqual(asset_requests.main(["--inbox", str(self.inbox), "status", "req-1", "accepted"]), 0)
        updated = asset_requests.read_records(self.inbox)[0]
        self.assertEqual(updated["status"], "accepted")
        self.assertEqual(
            {key: value for key, value in updated.items() if key != "status"},
            {key: value for key, value in original.items() if key != "status"},
        )
        self.assertEqual(updated["pipeline"], "human_review")

    def test_malformed_or_duplicate_inbox_fails_closed(self):
        malformed = '{"id":"valid"}\nnot json\n'
        self.inbox.write_text(malformed, encoding="utf-8")
        with self.assertRaises(asset_requests.InboxError):
            asset_requests.read_records(self.inbox)
        self.assertEqual(self.inbox.read_text(encoding="utf-8"), malformed)

        duplicate = '{"id":"same"}\n{"id":"same"}\n'
        self.inbox.write_text(duplicate, encoding="utf-8")
        with self.assertRaises(asset_requests.InboxError):
            asset_requests.read_records(self.inbox)
        self.assertEqual(self.inbox.read_text(encoding="utf-8"), duplicate)

    def test_missing_id_cannot_change_any_record(self):
        self.inbox.write_text('{"description":"no id"}\n', encoding="utf-8")
        with self.assertRaises(asset_requests.InboxError):
            asset_requests.read_records(self.inbox)


if __name__ == "__main__":
    unittest.main()
