import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest import mock


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "Preview-WoodlandFlyAgaricReview.py"
_previous_unreal = sys.modules.get("unreal")
_previous_stage = os.environ.pop("CAPTIVESKY_FUNGUS_REVIEW_STAGE", None)
_unreal_stub = types.ModuleType("unreal")
_unreal_stub.logs = []
_unreal_stub.errors = []
_unreal_stub.log = _unreal_stub.logs.append
_unreal_stub.log_error = _unreal_stub.errors.append
sys.modules["unreal"] = _unreal_stub
try:
    _spec = importlib.util.spec_from_file_location("woodland_fly_agaric_review", SCRIPT_PATH)
    review = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(review)
finally:
    if _previous_stage is not None:
        os.environ["CAPTIVESKY_FUNGUS_REVIEW_STAGE"] = _previous_stage
    if _previous_unreal is None:
        sys.modules.pop("unreal", None)
    else:
        sys.modules["unreal"] = _previous_unreal


class FakeMapAsset:
    def __init__(self, path):
        self.path = path


class FakeEditorAssetLibrary:
    def __init__(self, existing_assets=()):
        self.assets = set(existing_assets)
        self.directories = set()
        self.mutations = []

    def does_asset_exist(self, path):
        return path in self.assets

    def make_directory(self, path):
        self.mutations.append(("make_directory", path))
        self.directories.add(path)

    def does_directory_exist(self, path):
        return path in self.directories

    def duplicate_asset(self, source, destination):
        self.mutations.append(("duplicate_asset", source, destination))
        if source not in self.assets:
            return None
        self.assets.add(destination)
        return FakeMapAsset(destination)

    def save_loaded_asset(self, asset):
        self.mutations.append(("save_loaded_asset", asset.path))
        return True


class WoodlandFlyAgaricReviewTests(unittest.TestCase):
    def setUp(self):
        self.original_asset_library = getattr(review.unreal, "EditorAssetLibrary", None)
        self.original_logs = list(getattr(review.unreal, "logs", []))
        review.unreal.logs = []

    def tearDown(self):
        if self.original_asset_library is None:
            if hasattr(review.unreal, "EditorAssetLibrary"):
                delattr(review.unreal, "EditorAssetLibrary")
        else:
            review.unreal.EditorAssetLibrary = self.original_asset_library
        review.unreal.logs = self.original_logs

    def test_review_roster_compares_fungus_agaric_and_fern_without_collisions(self):
        self.assertEqual([item["name"] for item in review.CANDIDATES],
                         ["WoodlandFlyAgaricCluster", "WoodlandFernCluster"])
        self.assertEqual(len({item["mesh_path"] for item in review.CANDIDATES}), 2)
        self.assertEqual(len({item["actor_label"] for item in review.CANDIDATES}), 2)
        self.assertTrue(all(item["mesh_path"].startswith("/Game/Generated/ComfyBlender/")
                            for item in review.CANDIDATES))
        offsets = [item["offset_x_cm"] for item in review.CANDIDATES]
        self.assertGreater(offsets[0], 0)
        self.assertLess(offsets[1], 0)

    def test_import_preflights_both_candidate_sources_before_any_import(self):
        with tempfile.TemporaryDirectory() as root:
            first = review.CANDIDATES[0]
            asset_dir = Path(root, "Saved", "CaptiveSky", "ComfyBlender", first["name"])
            asset_dir.mkdir(parents=True)
            (asset_dir / first["filename"]).touch()

            previous_project_root = review.project_root
            review.project_root = lambda: root
            review.unreal.EditorAssetLibrary = FakeEditorAssetLibrary()
            try:
                with mock.patch.object(review.runpy, "run_path") as run_import:
                    with self.assertRaisesRegex(RuntimeError, "Verified candidate FBX is missing"):
                        review.import_candidates()
                    run_import.assert_not_called()
            finally:
                review.project_root = previous_project_root

    def test_import_never_overwrites_an_existing_unvalidated_mesh(self):
        existing_path = review.CANDIDATES[0]["mesh_path"]
        review.unreal.EditorAssetLibrary = FakeEditorAssetLibrary((existing_path,))
        with tempfile.TemporaryDirectory() as root:
            for candidate in review.CANDIDATES:
                asset_dir = Path(root, "Saved", "CaptiveSky", "ComfyBlender", candidate["name"])
                asset_dir.mkdir(parents=True)
                (asset_dir / candidate["filename"]).touch()

            previous_project_root = review.project_root
            review.project_root = lambda: root
            try:
                with mock.patch.object(review.unreal, "load_asset", return_value=None, create=True):
                    with mock.patch.object(review.runpy, "run_path") as run_import:
                        with self.assertRaisesRegex(RuntimeError, "Imported candidate Static Mesh is missing"):
                            review.import_candidates()
                        run_import.assert_not_called()
            finally:
                review.project_root = previous_project_root

    def test_duplicate_refuses_to_overwrite_an_existing_review_map(self):
        library = FakeEditorAssetLibrary((review.SOURCE_MAP, review.DEST_MAP))
        review.unreal.EditorAssetLibrary = library

        with self.assertRaisesRegex(RuntimeError, "Refusing to overwrite"):
            review.duplicate_island()

        self.assertEqual(library.mutations, [])

    def test_duplicate_saves_only_a_new_copy_of_the_source_map(self):
        library = FakeEditorAssetLibrary((review.SOURCE_MAP,))
        review.unreal.EditorAssetLibrary = library

        review.duplicate_island()

        self.assertIn(("duplicate_asset", review.SOURCE_MAP, review.DEST_MAP), library.mutations)
        saved_maps = [call[1] for call in library.mutations if call[0] == "save_loaded_asset"]
        self.assertEqual(saved_maps, [review.DEST_MAP])
        self.assertNotIn(review.SOURCE_MAP, saved_maps)
        self.assertIn(review.DEST_MAP, library.assets)

    def test_duplicate_does_not_mutate_when_source_map_is_missing(self):
        library = FakeEditorAssetLibrary()
        review.unreal.EditorAssetLibrary = library

        with self.assertRaisesRegex(RuntimeError, "Source Island map is missing"):
            review.duplicate_island()

        self.assertEqual(library.mutations, [])


if __name__ == "__main__":
    unittest.main()
