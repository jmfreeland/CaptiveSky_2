import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


PIPELINE = Path(__file__).resolve().parents[1]
REPO_ROOT = PIPELINE.parents[1]
TEST_TEMP_ROOT = REPO_ROOT / "Saved" / "Tests" / "AssetPipelineBuildLock"
if str(PIPELINE) not in sys.path:
    sys.path.insert(0, str(PIPELINE))

from build_lock import BuildLockError, pipeline_build_lock  # noqa: E402

MAKE_SPEC = importlib.util.spec_from_file_location("asset_pipeline_make", PIPELINE / "make.py")
make = importlib.util.module_from_spec(MAKE_SPEC)
MAKE_SPEC.loader.exec_module(make)


@unittest.skipUnless(sys.platform == "win32", "The asset pipeline lock uses Windows file-sharing semantics")
class AssetPipelineBuildLockTests(unittest.TestCase):
    def test_lock_excludes_a_second_owner_and_releases_after_build(self):
        TEST_TEMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEST_TEMP_ROOT) as directory:
            lock_path = Path(directory) / "asset-pipeline.lock"

            with pipeline_build_lock(lock_path):
                with self.assertRaisesRegex(BuildLockError, "Another asset build"):
                    with pipeline_build_lock(lock_path, timeout_seconds=0):
                        self.fail("A second build must not enter the critical section")

            with pipeline_build_lock(lock_path):
                pass

    def test_make_holds_the_shared_lock_across_each_asset_build(self):
        TEST_TEMP_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEST_TEMP_ROOT) as directory:
            lock_path = Path(directory) / "asset-pipeline.lock"
            built = []

            def record_build(name):
                with self.assertRaises(BuildLockError):
                    with pipeline_build_lock(lock_path, timeout_seconds=0):
                        self.fail("The orchestrator must keep its lock while building")
                built.append(name)

            with patch.object(make, "PIPELINE_LOCK_PATH", str(lock_path)), \
                    patch.object(make, "build_one", side_effect=record_build):
                self.assertEqual(make.main(["first", "second"]), 0)

            self.assertEqual(built, ["first", "second"])
            with pipeline_build_lock(lock_path):
                pass


if __name__ == "__main__":
    unittest.main()
