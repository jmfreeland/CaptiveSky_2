"""Source parity checks stay read-only and confined to project roots."""
from pathlib import Path
import runpy
import sys
import tempfile
import unittest


sys.dont_write_bytecode = True
SCRIPT = Path(__file__).resolve().parents[1] / "compare_project_source_parity.py"
PARITY = runpy.run_path(str(SCRIPT), run_name="source_parity")


class ProjectSourceParityTests(unittest.TestCase):
    def test_identical_different_and_missing_files_are_classified(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_root = root / "source"
            live_root = root / "live"
            (source_root / "Config").mkdir(parents=True)
            (live_root / "Config").mkdir(parents=True)
            (source_root / "Config" / "same.ini").write_text("same", encoding="utf-8")
            (live_root / "Config" / "same.ini").write_text("same", encoding="utf-8")
            (source_root / "Config" / "changed.ini").write_text("new", encoding="utf-8")
            (live_root / "Config" / "changed.ini").write_text("old", encoding="utf-8")
            (source_root / "Config" / "missing-live.ini").write_text("source", encoding="utf-8")
            (live_root / "Config" / "missing-source.ini").write_text("live", encoding="utf-8")

            rows = PARITY["compare_paths"](
                source_root,
                live_root,
                [
                    "Config/same.ini",
                    "Config/changed.ini",
                    "Config/missing-live.ini",
                    "Config/missing-source.ini",
                ],
            )

        self.assertEqual(
            rows,
            [
                ("identical", "Config/same.ini"),
                ("different", "Config/changed.ini"),
                ("live-missing", "Config/missing-live.ini"),
                ("source-missing", "Config/missing-source.ini"),
            ],
        )

    def test_windows_and_posix_escape_paths_are_rejected(self):
        for value in ("../outside.ini", "..\\outside.ini", "C:\\outside.ini", "\\\\server\\share\\outside.ini"):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    PARITY["_safe_relative_path"](value)

    def test_tracked_runtime_list_includes_project_zen_config(self):
        worktree_root = SCRIPT.parents[1]
        paths = PARITY["tracked_runtime_sources"](worktree_root)

        self.assertIn("Config/DefaultEngine.ini", paths)


if __name__ == "__main__":
    unittest.main()
