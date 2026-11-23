"""Source contract for the local-only Tideglass micro-scene study."""

import ast
import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "Scripts" / "AssetPipeline" / "reviews" / "tidepool_micro_scene.py"
CRAB = ROOT / "Saved" / "CaptiveSky" / "ComfyBlender" / "TidepoolCrab"


class TidepoolMicroSceneTests(unittest.TestCase):
    def test_study_is_bounded_and_uses_the_existing_authored_crab(self):
        source = SCRIPT.read_text(encoding="utf-8")
        tree = ast.parse(source)
        assignments = {}
        for node in tree.body:
            if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
                try:
                    assignments[node.targets[0].id] = ast.literal_eval(node.value)
                except (ValueError, TypeError):
                    pass
        self.assertEqual(assignments["PATCH_SIZE_M"], 4.0)
        self.assertEqual(assignments["CAMERA_HEIGHT_M"], 0.62)
        manifest = json.loads((CRAB / "manifest.json").read_text(encoding="utf-8"))
        self.assertTrue((CRAB / manifest["fbx"]).is_file())
        self.assertIn("unreal_validation", source)
        self.assertIn("ReviewOnly_", source)
        self.assertNotIn("/Game/Maps/Island", source)


if __name__ == "__main__":
    unittest.main()
