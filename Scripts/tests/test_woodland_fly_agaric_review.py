import importlib.util
import os
from pathlib import Path
import sys
import types
import unittest


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
    pass


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
        return FakeMapAsset()

    def save_loaded_asset(self, asset):
        self.mutations.append(("save_loaded_asset", asset))
        return True


class WoodlandFlyAgaricReviewTests(unittest.TestCase):
    def setUp(self):
        self.original_asset_library = getattr(review.unreal, "EditorAssetLibrary", None)
        self.original_logs = list(getattr(review.unreal, "logs", []))
        review.unreal.logs = []

    def tearDown(self):
        if self.original_asset_library is None:
            delattr(review.unreal, "EditorAssetLibrary")
        else:
            review.unreal.EditorAssetLibrary = self.original_asset_library
        review.unreal.logs = self.original_logs

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
        self.assertTrue(any(call[0] == "save_loaded_asset" for call in library.mutations))
        self.assertFalse(any(call[0] == "save_source_map" for call in library.mutations))
        self.assertIn(review.DEST_MAP, library.assets)

    def test_duplicate_does_not_mutate_when_source_map_is_missing(self):
        library = FakeEditorAssetLibrary()
        review.unreal.EditorAssetLibrary = library

        with self.assertRaisesRegex(RuntimeError, "Source Island map is missing"):
            review.duplicate_island()

        self.assertEqual(library.mutations, [])


if __name__ == "__main__":
    unittest.main()
