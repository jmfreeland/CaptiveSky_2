import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch


PROJECT_ROOT = Path(__file__).resolve().parents[2]
SCRATCH_ROOT = PROJECT_ROOT / "Saved" / "GatewayCheckBuild"
SCRIPT_PATH = PROJECT_ROOT / "Scripts" / "Import-ComfyBlenderAsset.py"


def load_importer():
    logs = []
    unreal_stub = types.ModuleType("unreal")
    unreal_stub.Paths = types.SimpleNamespace(project_dir=lambda: str(PROJECT_ROOT))
    unreal_stub.log = logs.append
    unreal_stub.log_error = logs.append
    previous_unreal = sys.modules.get("unreal")
    sys.modules["unreal"] = unreal_stub
    try:
        with tempfile.TemporaryDirectory(dir=SCRATCH_ROOT) as source_root, patch.dict(os.environ, {
            "COMFYBLENDER_ASSET": "MissingImporterTestAsset",
            "COMFYBLENDER_DESTINATION": "MissingImporterTestAsset",
            "COMFYBLENDER_SOURCE_ROOT": source_root,
        }):
            spec = importlib.util.spec_from_file_location("comfyblender_import_under_test", SCRIPT_PATH)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
    finally:
        if previous_unreal is None:
            sys.modules.pop("unreal", None)
        else:
            sys.modules["unreal"] = previous_unreal
    return module, unreal_stub, logs


IMPORTER, UNREAL, IMPORT_LOGS = load_importer()


class FakeVector:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z

    def __sub__(self, other):
        return FakeVector(self.x - other.x, self.y - other.y, self.z - other.z)


class FakeMesh:
    def __init__(self, size):
        self.size = size

    def get_bounding_box(self):
        return types.SimpleNamespace(min=FakeVector(0, 0, 0), max=FakeVector(*self.size))


class ComfyBlenderImporterTests(unittest.TestCase):
    def test_shared_library_texture_resolves_inside_the_generated_source_root(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH_ROOT) as temp_dir:
            root = Path(temp_dir)
            asset_dir = root / "Asset"
            texture = root / "Library" / "Textures" / "T_Lib_Wood_BC.png"
            texture.parent.mkdir(parents=True)
            texture.write_bytes(b"texture fixture")
            original_root = IMPORTER.SOURCE_ROOT
            try:
                IMPORTER.SOURCE_ROOT = str(root)
                resolved = IMPORTER.texture_path(
                    str(asset_dir), "../Library/Textures/T_Lib_Wood_BC.png (sRGB, tiles)")
            finally:
                IMPORTER.SOURCE_ROOT = original_root
        self.assertEqual(resolved, str(texture.resolve()))

    def test_texture_reference_cannot_escape_the_generated_source_root(self):
        with tempfile.TemporaryDirectory(dir=SCRATCH_ROOT) as temp_dir:
            root = Path(temp_dir)
            asset_dir = root / "Asset"
            outside = root.parent / (root.name + "-outside.png")
            outside.write_bytes(b"outside fixture")
            original_root = IMPORTER.SOURCE_ROOT
            try:
                IMPORTER.SOURCE_ROOT = str(root)
                reference = os.path.relpath(outside, asset_dir)
                with self.assertRaisesRegex(ValueError, "escapes the ComfyBlender library"):
                    IMPORTER.texture_path(str(asset_dir), reference)
            finally:
                IMPORTER.SOURCE_ROOT = original_root
                outside.unlink(missing_ok=True)

    def test_same_name_mesh_reimport_is_refused_before_creating_an_import_task(self):
        original_load_asset = UNREAL.load_asset if hasattr(UNREAL, "load_asset") else None
        original_import_task = getattr(UNREAL, "AssetImportTask", None)
        UNREAL.load_asset = lambda _path: object()
        UNREAL.AssetImportTask = lambda: self.fail("in-place reimport must stop before creating a task")
        try:
            with self.assertRaisesRegex(RuntimeError, "Refusing in-place static-mesh reimport"):
                IMPORTER.import_mesh("/tmp/SM_WoodlandPlant.fbx", "/Game/Generated/ComfyBlender/Plant")
        finally:
            if original_load_asset is None:
                del UNREAL.load_asset
            else:
                UNREAL.load_asset = original_load_asset
            if original_import_task is None:
                del UNREAL.AssetImportTask
            else:
                UNREAL.AssetImportTask = original_import_task

    def test_import_report_rejects_scale_material_and_collision_mismatches(self):
        manifest = {
            "size_cm_xyz": [198, 54, 47],
            "material_slots": ["M_Wood", "M_Moss"],
            "collision": ["UCX_01"],
        }
        assignments = [{"slot": "M_Wood", "material": "/Game/Materials/M_Wood"}]
        collisions = {"box_elems": 0, "sphere_elems": 0, "sphyl_elems": 0, "convex_elems": 0}
        errors = IMPORTER.verify_import(manifest, FakeMesh([2, 54, 47]), assignments, collisions)
        self.assertEqual(len(errors), 3)
        self.assertTrue(any("scale mismatch" in error for error in errors))
        self.assertTrue(any("No imported material" in error for error in errors))
        self.assertTrue(any("Collision count mismatch" in error for error in errors))

    def test_import_bootstrap_reports_missing_fixture_without_importing_assets(self):
        self.assertTrue(any("MissingImporterTestAsset" in message for message in IMPORT_LOGS))
        self.assertFalse(any("AssetImportTask" in message for message in IMPORT_LOGS))


if __name__ == "__main__":
    unittest.main()
