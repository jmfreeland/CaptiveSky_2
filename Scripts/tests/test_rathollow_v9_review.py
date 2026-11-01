"""Fail-closed tests for the staged, copied-map RatHollow V9 review."""
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import types
import unittest
from unittest import mock


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "Preview-RatHollowV9Review.py"
_previous_unreal = sys.modules.get("unreal")
_previous_stage = os.environ.pop("CAPTIVESKY_RATHOLLOW_REVIEW_STAGE", None)
_unreal_stub = types.ModuleType("unreal")
_unreal_stub.logs = []
_unreal_stub.errors = []
_unreal_stub.log = _unreal_stub.logs.append
_unreal_stub.log_error = _unreal_stub.errors.append
_unreal_stub.StaticMesh = type("StaticMesh", (), {})
sys.modules["unreal"] = _unreal_stub
try:
    _spec = importlib.util.spec_from_file_location("rathollow_v9_review", SCRIPT_PATH)
    review = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(review)
finally:
    if _previous_stage is not None:
        os.environ["CAPTIVESKY_RATHOLLOW_REVIEW_STAGE"] = _previous_stage
    if _previous_unreal is None:
        sys.modules.pop("unreal", None)
    else:
        sys.modules["unreal"] = _previous_unreal


class FakeMapAsset:
    def __init__(self, path):
        self.path = path


class FakeAssetLibrary:
    def __init__(self, existing=()):
        self.assets = set(existing)
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


class RatHollowV9ReviewTests(unittest.TestCase):
    def setUp(self):
        self.original_asset_library = getattr(review.unreal, "EditorAssetLibrary", None)

    def tearDown(self):
        if self.original_asset_library is None:
            if hasattr(review.unreal, "EditorAssetLibrary"):
                delattr(review.unreal, "EditorAssetLibrary")
        else:
            review.unreal.EditorAssetLibrary = self.original_asset_library

    @staticmethod
    def write_source_pack(root):
        directory = Path(root, "Saved", "CaptiveSky", "ComfyBlender", review.ASSET_NAME)
        directory.mkdir(parents=True)
        (directory / review.FBX_NAME).write_bytes(b"fbx")
        manifest = {
            "asset": review.MESH_NAME,
            "collision": ["UBX_{}_{:02d}".format(review.MESH_NAME, index) for index in range(1, 6)],
            "anchors_cm": {"entrance": [-132.0, 0.0, 66.0]},
        }
        (directory / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        return manifest

    def test_source_pack_requires_all_authored_collision_boxes(self):
        with tempfile.TemporaryDirectory() as root:
            manifest = self.write_source_pack(root)
            self.assertEqual(review.load_source_manifest(root), manifest)
            manifest["collision"] = manifest["collision"][:-1]
            path = Path(root, "Saved", "CaptiveSky", "ComfyBlender", review.ASSET_NAME, "manifest.json")
            path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "five authored box collisions"):
                review.load_source_manifest(root)

    def test_site_must_match_a_qualified_site_audit_record(self):
        audit = "RatHollowSiteAudit candidate[2] pos=X=-105200.0 Y=104200.0 Z=2334.0 slope=18.0cm crownSamples=4/5 navFootprint=5/5."
        record = {
            "candidate_index": 2,
            "location_cm": [-105200, 104200, 2334],
            "rotation_degrees": [0, 90, 0],
        }
        location, rotation = review.parse_audited_site(audit, record)
        self.assertEqual(location, [-105200.0, 104200.0, 2334.0])
        self.assertEqual(rotation, [0.0, 90.0, 0.0])

    def test_site_rejects_near_misses_and_coordinates_not_in_the_audit(self):
        near_miss = "RatHollowSiteAudit near-miss[2] pos=X=1.0 Y=2.0 Z=3.0 slope=1.0cm."
        wrong_site = {
            "candidate_index": 2,
            "location_cm": [1, 2, 3],
            "rotation_degrees": [0, 0, 0],
        }
        with self.assertRaisesRegex(RuntimeError, "not a qualified candidate"):
            review.parse_audited_site(near_miss, wrong_site)

        candidate = "RatHollowSiteAudit candidate[2] pos=X=1.0 Y=2.0 Z=3.0 slope=1.0cm."
        with self.assertRaisesRegex(RuntimeError, "do not match"):
            review.parse_audited_site(candidate, {
                "candidate_index": 2,
                "location_cm": [9, 9, 9],
                "rotation_degrees": [0, 0, 0],
            })

    def test_site_record_must_reference_a_saved_audit_and_exact_candidate(self):
        with tempfile.TemporaryDirectory() as root:
            project = Path(root, "Project")
            audit = project / "Saved" / "Logs" / "RatHollowSiteAudit.log"
            audit.parent.mkdir(parents=True)
            audit.write_text(
                "Island.RatHollowSiteAudit scan center=(0,0)\n"
                "RatHollowSiteAudit candidate[2] pos=X=-105200.0 Y=104200.0 Z=2334.0 slope=18.0cm.\n",
                encoding="utf-8",
            )
            review_dir = project / review.REVIEW_DATA_RELATIVE
            review_dir.mkdir(parents=True)
            site = {
                "site_id": "qualified-2",
                "candidate_index": 2,
                "location_cm": [-105200, 104200, 2334],
                "rotation_degrees": [0, 90, 0],
                "audit_log": "Saved/Logs/RatHollowSiteAudit.log",
            }
            (review_dir / "site.json").write_text(json.dumps(site), encoding="utf-8")

            with mock.patch.object(review, "project_root", return_value=str(project)):
                record, audit_relative, location, rotation = review.read_review_site()
            self.assertEqual(record["site_id"], "qualified-2")
            self.assertEqual(audit_relative, site["audit_log"])
            self.assertEqual(location, [-105200.0, 104200.0, 2334.0])
            self.assertEqual(rotation, [0.0, 90.0, 0.0])

            site["audit_log"] = "Scripts/not-a-saved-audit.log"
            (review_dir / "site.json").write_text(json.dumps(site), encoding="utf-8")
            with mock.patch.object(review, "project_root", return_value=str(project)):
                with self.assertRaisesRegex(RuntimeError, "inside the active project's Saved tree"):
                    review.read_review_site()

            site["audit_log"] = "Saved/Logs/RatHollowSiteAudit.log"
            (review_dir / "site.json").write_text(json.dumps(site), encoding="utf-8")
            os.utime(audit, (time.time() - 172800, time.time() - 172800))
            with mock.patch.object(review, "project_root", return_value=str(project)):
                with self.assertRaisesRegex(RuntimeError, "no more than 24 hours old"):
                    review.read_review_site()

    def test_duplicate_saves_only_a_new_map_copy(self):
        library = FakeAssetLibrary((review.SOURCE_MAP,))
        review.unreal.EditorAssetLibrary = library

        review.duplicate_island()

        self.assertIn(("duplicate_asset", review.SOURCE_MAP, review.DEST_MAP), library.mutations)
        saved = [entry[1] for entry in library.mutations if entry[0] == "save_loaded_asset"]
        self.assertEqual(saved, [review.DEST_MAP])
        self.assertNotIn(review.SOURCE_MAP, saved)

    def test_duplicate_refuses_existing_destination_before_mutation(self):
        library = FakeAssetLibrary((review.SOURCE_MAP, review.DEST_MAP))
        review.unreal.EditorAssetLibrary = library

        with self.assertRaisesRegex(RuntimeError, "Refusing to overwrite"):
            review.duplicate_island()

        self.assertEqual(library.mutations, [])

    def test_placement_refuses_the_canonical_island(self):
        class Outer:
            def get_name(self):
                return review.SOURCE_MAP

        class World:
            def get_outermost(self):
                return Outer()

        class ActorTools:
            spawned = 0

            def get_all_level_actors(self):
                return []

            def spawn_actor_from_object(self, *args):
                self.spawned += 1

        tools = ActorTools()
        with tempfile.TemporaryDirectory() as root:
            self.write_source_pack(root)
            with mock.patch.object(review, "checkout_root", return_value=root), \
                    mock.patch.object(review, "load_source_manifest", return_value={"collision": [1] * 5}), \
                    mock.patch.object(review, "validate_imported_mesh", return_value=object()), \
                    mock.patch.object(review.unreal, "EditorLevelLibrary",
                                      types.SimpleNamespace(get_editor_world=lambda: World()), create=True), \
                    mock.patch.object(review.unreal, "EditorActorSubsystem", object, create=True), \
                    mock.patch.object(review.unreal, "get_editor_subsystem", return_value=tools, create=True) as get_subsystem:
                with self.assertRaisesRegex(RuntimeError, "Open only the isolated RatHollow review map"):
                    review.place_candidate()
                get_subsystem.assert_not_called()
        self.assertEqual(tools.spawned, 0)

    def test_placement_requires_an_explicit_audited_site_before_spawning(self):
        class Outer:
            def get_name(self):
                return review.DEST_MAP

        class World:
            def get_outermost(self):
                return Outer()

        class ActorTools:
            def get_all_level_actors(self):
                return []

            def spawn_actor_from_object(self, *args):
                raise AssertionError("Placement must not spawn before a reviewed site exists")

        with tempfile.TemporaryDirectory() as root:
            source = Path(root, "Source")
            project = Path(root, "Project")
            source.mkdir()
            project.mkdir()
            with mock.patch.object(review, "checkout_root", return_value=str(source)), \
                    mock.patch.object(review, "load_source_manifest", return_value={"collision": [1] * 5}), \
                    mock.patch.object(review, "validate_imported_mesh", return_value=object()), \
                    mock.patch.object(review, "project_root", return_value=str(project)), \
                    mock.patch.object(review.unreal, "EditorLevelLibrary",
                                      types.SimpleNamespace(get_editor_world=lambda: World()), create=True), \
                    mock.patch.object(review.unreal, "EditorActorSubsystem", object, create=True), \
                    mock.patch.object(review.unreal, "get_editor_subsystem", return_value=ActorTools(), create=True) as get_subsystem:
                with self.assertRaisesRegex(RuntimeError, "No RatHollow review site selected"):
                    review.place_candidate()
                get_subsystem.assert_not_called()


if __name__ == "__main__":
    unittest.main()
