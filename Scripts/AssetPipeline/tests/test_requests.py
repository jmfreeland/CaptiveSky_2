import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import subprocess
import sys


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "requests.py"
TEST_TEMP_ROOT = Path(__file__).resolve().parents[3] / "Saved" / "AssetPipelineTests"
SPEC = importlib.util.spec_from_file_location("asset_requests", SCRIPT_PATH)
asset_requests = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(asset_requests)


class RequestInboxTests(unittest.TestCase):
    def setUp(self):
        TEST_TEMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(prefix="requests-", dir=TEST_TEMP_ROOT)
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

    @unittest.skipUnless(asset_requests.os.name == "nt", "The Unreal/Discord inbox lock is Windows-specific")
    def test_review_status_update_fails_closed_while_shared_inbox_lock_is_held(self):
        original = {
            "id": "req-locked", "requester": "Aster", "description": "A sheltered fern patch",
            "request_type": "upgrade", "target": "WoodlandEdge", "upgrade_kind": "variation",
            "pipeline": "human_review", "status": "pending_review",
        }
        self.inbox.write_text(json.dumps(original) + "\n", encoding="utf-8")
        before = self.inbox.read_text(encoding="utf-8")

        with asset_requests.inbox_lock(self.inbox):
            with patch.object(asset_requests, "INBOX_LOCK_TIMEOUT_SECONDS", 0):
                self.assertEqual(asset_requests.main([
                    "--inbox", str(self.inbox), "status", "req-locked", "accepted"
                ]), 2)
            self.assertEqual(self.inbox.read_text(encoding="utf-8"), before)

        self.assertEqual(asset_requests.main([
            "--inbox", str(self.inbox), "status", "req-locked", "accepted"
        ]), 0)
        updated = asset_requests.read_records(self.inbox)[0]
        self.assertEqual(updated["status"], "accepted")
        self.assertEqual(updated["target"], original["target"])

    def test_request_must_be_accepted_before_generation(self):
        self.inbox.write_text(
            json.dumps({"id": "req-1", "status": "pending_review", "description": "A sheltered perch"}) + "\n",
            encoding="utf-8",
        )
        before = self.inbox.read_text(encoding="utf-8")
        self.assertEqual(asset_requests.main(["--inbox", str(self.inbox), "status", "req-1", "generated"]), 2)
        self.assertEqual(self.inbox.read_text(encoding="utf-8"), before)

    def test_accepted_request_can_be_generated_but_terminal_status_cannot_change(self):
        self.inbox.write_text(
            json.dumps({"id": "req-1", "status": "accepted", "description": "A sheltered perch"}) + "\n",
            encoding="utf-8",
        )
        self.assertEqual(asset_requests.main(["--inbox", str(self.inbox), "status", "req-1", "generated"]), 0)
        generated = self.inbox.read_text(encoding="utf-8")
        self.assertEqual(asset_requests.main(["--inbox", str(self.inbox), "status", "req-1", "declined"]), 2)
        self.assertEqual(self.inbox.read_text(encoding="utf-8"), generated)

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

    def test_accepted_new_object_runs_only_the_named_authored_script(self):
        original = {
            "id": "req-build", "requester": "Raven", "description": "A weatherproof nest cover",
            "request_type": "new_object", "pipeline": "ComfyBlender", "status": "accepted",
        }
        self.inbox.write_text(json.dumps(original) + "\n", encoding="utf-8")
        success_output = "[rathollow] build rc=0 exported=True\nPROBLEMS []\nVERIFY DONE\n"
        with patch.object(asset_requests.subprocess, "run", return_value=subprocess.CompletedProcess(
            [], 0, stdout=success_output, stderr=""
        )) as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "req-build", "rathollow"
            ]), 0)
        self.assertEqual(run.call_args.args[0], [
            sys.executable, "-I", str(asset_requests.MAKE_SCRIPT), "rathollow"
        ])
        self.assertEqual(run.call_args.kwargs["cwd"], asset_requests.REPO_ROOT)
        self.assertEqual(asset_requests.read_records(self.inbox)[0], original)

    def test_pending_or_upgrade_request_never_launches_asset_pipeline(self):
        records = [
            {"id": "pending", "requester": "Aster", "description": "A sign",
             "request_type": "new_object", "pipeline": "ComfyBlender", "status": "pending_review"},
            {"id": "upgrade", "requester": "Raven", "description": "Add a perch",
             "request_type": "upgrade", "pipeline": "human_review", "upgrade_kind": "functionality",
             "status": "accepted"},
        ]
        self.inbox.write_text("".join(json.dumps(record) + "\n" for record in records), encoding="utf-8")
        with patch.object(asset_requests.subprocess, "run") as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "pending", "rathollow"
            ]), 2)
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "upgrade", "rathollow"
            ]), 2)
        run.assert_not_called()
        self.assertEqual(asset_requests.read_records(self.inbox), records)

    def test_accepted_visual_upgrade_needs_explicit_route_then_uses_authored_script(self):
        request = {"id": "upgrade-1", "requester": "Raven", "description": "Give the Wind Arch mossy stones",
                   "request_type": "upgrade", "target": "WindArch", "upgrade_kind": "aesthetic",
                   "pipeline": "human_review", "status": "accepted"}
        self.inbox.write_text(json.dumps(request) + "\n", encoding="utf-8")
        success_output = "[rathollow] build rc=0 exported=True\nPROBLEMS []\nVERIFY DONE\n"
        with patch.object(asset_requests.subprocess, "run", return_value=subprocess.CompletedProcess(
            [], 0, stdout=success_output, stderr=""
        )) as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "upgrade-1", "rathollow"
            ]), 2)
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "route", "upgrade-1", "ComfyBlender"
            ]), 0)
            routed = asset_requests.read_records(self.inbox)[0]
            self.assertEqual(routed["status"], "accepted")
            self.assertEqual(routed["pipeline"], "ComfyBlender")
            self.assertEqual(routed["target"], "WindArch")
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "upgrade-1", "rathollow"
            ]), 0)
        run.assert_called_once()

    def test_functionality_upgrade_cannot_be_routed_to_comfyblender(self):
        request = {"id": "upgrade-2", "requester": "Aster", "description": "Add a usable shelter",
                   "request_type": "upgrade", "upgrade_kind": "functionality",
                   "pipeline": "human_review", "status": "accepted"}
        self.inbox.write_text(json.dumps(request) + "\n", encoding="utf-8")
        with patch.object(asset_requests.subprocess, "run") as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "route", "upgrade-2", "ComfyBlender"
            ]), 2)
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "upgrade-2", "rathollow"
            ]), 2)
        run.assert_not_called()
        self.assertEqual(asset_requests.read_records(self.inbox)[0]["pipeline"], "human_review")

    def test_variation_upgrade_can_use_reviewed_asset_script(self):
        request = {"id": "upgrade-variation", "requester": "Aster",
                   "description": "Make a second, wind-bent variant of the existing perch",
                   "request_type": "upgrade", "target": "WindArchPerch",
                   "upgrade_kind": "variation", "pipeline": "human_review", "status": "accepted"}
        self.inbox.write_text(json.dumps(request) + "\n", encoding="utf-8")
        success_output = "[rathollow] build rc=0 exported=True\nPROBLEMS []\nVERIFY DONE\n"
        with patch.object(asset_requests.subprocess, "run", return_value=subprocess.CompletedProcess(
            [], 0, stdout=success_output, stderr=""
        )) as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "route", "upgrade-variation", "ComfyBlender"
            ]), 0)
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "upgrade-variation", "rathollow"
            ]), 0)
        run.assert_called_once()
        self.assertEqual(run.call_args.args[0][-1], "rathollow")
        routed = asset_requests.read_records(self.inbox)[0]
        self.assertEqual(routed["status"], "accepted")
        self.assertEqual(routed["target"], "WindArchPerch")
        self.assertEqual(routed["upgrade_kind"], "variation")
        self.assertEqual(routed["pipeline"], "ComfyBlender")

    def test_invalid_or_missing_asset_script_never_launches_pipeline(self):
        accepted = {"id": "req-build", "requester": "Raven", "description": "A cover",
                    "status": "accepted", "request_type": "new_object", "pipeline": "ComfyBlender"}
        self.inbox.write_text(json.dumps(accepted) + "\n", encoding="utf-8")
        with patch.object(asset_requests.subprocess, "run") as run:
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "req-build", "../rathollow"
            ]), 2)
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "req-build", "not_a_real_asset"
            ]), 2)
        run.assert_not_called()

    def test_pipeline_failure_keeps_request_accepted(self):
        accepted = {"id": "req-build", "requester": "Raven", "description": "A cover",
                    "status": "accepted", "request_type": "new_object", "pipeline": "ComfyBlender"}
        self.inbox.write_text(json.dumps(accepted) + "\n", encoding="utf-8")
        with patch.object(asset_requests.subprocess, "run", return_value=subprocess.CompletedProcess(
            [], 7, stdout="", stderr=""
        )):
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "req-build", "rathollow"
            ]), 7)
        self.assertEqual(asset_requests.read_records(self.inbox)[0]["status"], "accepted")

    def test_zero_exit_without_verifier_markers_is_not_reported_as_generated(self):
        accepted = {"id": "req-build", "requester": "Raven", "description": "A cover",
                    "status": "accepted", "request_type": "new_object", "pipeline": "ComfyBlender"}
        self.inbox.write_text(json.dumps(accepted) + "\n", encoding="utf-8")
        with patch.object(asset_requests.subprocess, "run", return_value=subprocess.CompletedProcess(
            [], 0, stdout="[rathollow] build rc=0 exported=False\n", stderr=""
        )):
            self.assertEqual(asset_requests.main([
                "--inbox", str(self.inbox), "build", "req-build", "rathollow"
            ]), 1)
        self.assertEqual(asset_requests.read_records(self.inbox)[0]["status"], "accepted")


if __name__ == "__main__":
    unittest.main()
