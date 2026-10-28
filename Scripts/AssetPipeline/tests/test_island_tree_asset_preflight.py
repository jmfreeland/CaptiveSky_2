import importlib.util
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).resolve().parents[2] / "check_island_tree_assets.py"
SPEC = importlib.util.spec_from_file_location("check_island_tree_assets", SCRIPT_PATH)
preflight = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(preflight)


class IslandTreeAssetPreflightTests(unittest.TestCase):
    def test_extracts_unique_megaplant_paths_and_ignores_other_assets(self):
        source = '''
            LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Megaplant_Library/Tree_A/Tree_A.Tree_A"));
            LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Megaplant_Library/Tree_A/Tree_A.Tree_A"));
            LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Plants/Tree_B.Tree_B"));
        '''
        self.assertEqual(
            preflight.referenced_tree_packages(source),
            ["/Game/Megaplant_Library/Tree_A/Tree_A.Tree_A"],
        )

    def test_maps_object_path_to_package_asset_file(self):
        root = Path("workspace")
        actual = preflight.package_file_for_reference(
            root, "/Game/Megaplant_Library/Tree_A/Tree_A.Tree_A"
        )
        self.assertEqual(
            actual,
            root / "Content/Megaplant_Library/Tree_A/Tree_A.uasset",
        )

    def test_rejects_paths_outside_megaplant_library(self):
        with self.assertRaises(ValueError):
            preflight.package_file_for_reference(Path("."), "/Game/Plants/Tree.Tree")


if __name__ == "__main__":
    unittest.main()
