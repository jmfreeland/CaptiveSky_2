import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "Place-IslandProps.py"
unreal_stub = sys.modules.setdefault("unreal", types.ModuleType("unreal"))
unreal_stub.Paths = types.SimpleNamespace(project_dir=lambda: str(SCRIPT_PATH.parents[2]))
SPEC = importlib.util.spec_from_file_location("place_island_props", SCRIPT_PATH)
place_island_props = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(place_island_props)


class FakeVector:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z


class FakeHit:
    def to_tuple(self):
        return (FakeVector(0, 0, 1200),)


class FakeMesh:
    def get_bounding_box(self):
        return types.SimpleNamespace(min=FakeVector(-120, -60, 0), max=FakeVector(120, 60, 130))


class FakeEditorLevelLibrary:
    def __init__(self, actors=None):
        self.mutations = []
        self.actors = list(actors or [])
        self.actor_reads = 0

    def load_level(self, _path):
        pass

    def get_editor_world(self):
        return object()

    def get_all_level_actors(self):
        self.actor_reads += 1
        return self.actors

    def destroy_actor(self, actor):
        self.mutations.append(("destroy", actor))

    def spawn_actor_from_class(self, *args):
        self.mutations.append(("spawn", args))
        actor = FakeStaticMeshActor()
        self.actors.append(actor)
        return actor

    def save_current_level(self):
        self.mutations.append(("save",))


class FakeComponent:
    def __init__(self):
        self.mesh = None

    def set_mobility(self, _mobility):
        pass

    def set_static_mesh(self, mesh):
        self.mesh = mesh


class FakeStaticMeshActor:
    def __init__(self, tags=None):
        self.tags = list(tags or [])
        self.static_mesh_component = FakeComponent()
        self.label = ""
        self.location = None
        self.rotation = None

    def get_actor_label(self):
        return self.label

    def set_actor_label(self, label):
        self.label = label

    def set_actor_location_and_rotation(self, location, rotation, _sweep, _teleport):
        self.location = location
        self.rotation = rotation


def fake_unreal(mesh, actors=None):
    level = FakeEditorLevelLibrary(actors)
    engine = types.SimpleNamespace(
        Paths=types.SimpleNamespace(project_dir=lambda: ""),
        EditorLevelLibrary=level,
        SystemLibrary=types.SimpleNamespace(line_trace_single=lambda *args: FakeHit()),
        TraceTypeQuery=types.SimpleNamespace(TRACE_TYPE_QUERY1=1),
        DrawDebugTrace=types.SimpleNamespace(NONE=0),
        Vector=FakeVector,
        Actor=type("Actor", (), {}),
        StaticMeshActor=FakeStaticMeshActor,
        ComponentMobility=types.SimpleNamespace(STATIC=0),
        Rotator=lambda **kwargs: types.SimpleNamespace(**kwargs),
        load_asset=lambda _path: mesh,
        log=lambda _message: None,
    )
    return engine, level


class GeneratedPropPathTests(unittest.TestCase):
    def test_explicit_asset_path_supports_non_tripo_sources(self):
        path = "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01"
        self.assertEqual(place_island_props.mesh_asset_path({"asset_path": path}), path)

    def test_asset_name_keeps_legacy_tripo_convention(self):
        self.assertEqual(
            place_island_props.mesh_asset_path({"asset": "FirewoodStack"}),
            "/Game/Generated/Tripo/FirewoodStack/FirewoodStack/StaticMeshes/FirewoodStack",
        )

    def test_missing_asset_reference_is_rejected(self):
        with self.assertRaises(ValueError):
            place_island_props.mesh_asset_path({"id": "RatHollow"})

    def test_default_run_is_preview_only(self):
        engine, level = fake_unreal(FakeMesh())
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
        original_dry_run = place_island_props.os.environ.get("PLACE_PROPS_DRYRUN")
        original_apply = place_island_props.os.environ.get("PLACE_PROPS_APPLY")
        try:
            place_island_props.unreal = engine
            with tempfile.TemporaryDirectory() as temp_dir:
                config_path = Path(temp_dir) / "props.json"
                config_path.write_text(json.dumps({"props": [{
                    "id": "RatHollow",
                    "asset_path": "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01",
                    "x": -105000,
                    "y": 104750,
                }]}), encoding="utf-8")
                place_island_props.CONFIG = str(config_path)
                place_island_props.os.environ.pop("PLACE_PROPS_DRYRUN", None)
                place_island_props.os.environ.pop("PLACE_PROPS_APPLY", None)
                place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            if original_dry_run is None:
                place_island_props.os.environ.pop("PLACE_PROPS_DRYRUN", None)
            else:
                place_island_props.os.environ["PLACE_PROPS_DRYRUN"] = original_dry_run
            if original_apply is None:
                place_island_props.os.environ.pop("PLACE_PROPS_APPLY", None)
            else:
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = original_apply
        self.assertEqual(level.mutations, [])
        self.assertEqual(level.actor_reads, 0)

    def test_explicit_dry_run_overrides_apply_opt_in(self):
        engine, level = fake_unreal(FakeMesh())
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
        original_apply = place_island_props.os.environ.get("PLACE_PROPS_APPLY")
        original_dry_run = place_island_props.os.environ.get("PLACE_PROPS_DRYRUN")
        try:
            place_island_props.unreal = engine
            with tempfile.TemporaryDirectory() as temp_dir:
                config_path = Path(temp_dir) / "props.json"
                config_path.write_text(json.dumps({"props": [{
                    "id": "RatHollow",
                    "asset_path": "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01",
                    "x": -105000,
                    "y": 104750,
                }]}), encoding="utf-8")
                place_island_props.CONFIG = str(config_path)
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = "1"
                place_island_props.os.environ["PLACE_PROPS_DRYRUN"] = "1"
                place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            for name, original in (("PLACE_PROPS_APPLY", original_apply), ("PLACE_PROPS_DRYRUN", original_dry_run)):
                if original is None:
                    place_island_props.os.environ.pop(name, None)
                else:
                    place_island_props.os.environ[name] = original
        self.assertEqual(level.mutations, [])
        self.assertEqual(level.actor_reads, 0)

    def test_apply_updates_only_matching_id_and_leaves_other_props_untouched(self):
        target = FakeStaticMeshActor([place_island_props.TAG, "RatHollow"])
        unrelated = FakeStaticMeshActor([place_island_props.TAG, "InnLanternPost_East"])
        legacy_unrelated = FakeStaticMeshActor([place_island_props.LEGACY_TAG, "OldLantern"])
        engine, level = fake_unreal(FakeMesh(), [target, unrelated, legacy_unrelated])
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
        original_apply = place_island_props.os.environ.get("PLACE_PROPS_APPLY")
        original_dry_run = place_island_props.os.environ.get("PLACE_PROPS_DRYRUN")
        try:
            place_island_props.unreal = engine
            with tempfile.TemporaryDirectory() as temp_dir:
                config_path = Path(temp_dir) / "props.json"
                config_path.write_text(json.dumps({"props": [{
                    "id": "RatHollow",
                    "asset_path": "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01",
                    "x": -105000,
                    "y": 104750,
                }]}), encoding="utf-8")
                place_island_props.CONFIG = str(config_path)
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = "1"
                place_island_props.os.environ.pop("PLACE_PROPS_DRYRUN", None)
                place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            for name, original in (("PLACE_PROPS_APPLY", original_apply), ("PLACE_PROPS_DRYRUN", original_dry_run)):
                if original is None:
                    place_island_props.os.environ.pop(name, None)
                else:
                    place_island_props.os.environ[name] = original
        self.assertEqual(level.actor_reads, 1)
        self.assertEqual([entry[0] for entry in level.mutations], ["save"])
        self.assertEqual(target.static_mesh_component.mesh, engine.load_asset("ignored"))
        self.assertEqual(target.label, "Prop_RatHollow")
        self.assertEqual(target.tags, [place_island_props.TAG, "RatHollow"])
        self.assertIsNone(unrelated.location)
        self.assertIsNone(legacy_unrelated.location)

    def test_duplicate_existing_ids_abort_before_any_actor_changes(self):
        duplicate_a = FakeStaticMeshActor([place_island_props.TAG, "RatHollow"])
        duplicate_b = FakeStaticMeshActor([place_island_props.LEGACY_TAG, "RatHollow"])
        engine, level = fake_unreal(FakeMesh(), [duplicate_a, duplicate_b])
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
        original_apply = place_island_props.os.environ.get("PLACE_PROPS_APPLY")
        try:
            place_island_props.unreal = engine
            with tempfile.TemporaryDirectory() as temp_dir:
                config_path = Path(temp_dir) / "props.json"
                config_path.write_text(json.dumps({"props": [{
                    "id": "RatHollow",
                    "asset_path": "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01",
                    "x": -105000,
                    "y": 104750,
                }]}), encoding="utf-8")
                place_island_props.CONFIG = str(config_path)
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = "1"
                with self.assertRaisesRegex(RuntimeError, "no actors were changed"):
                    place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            if original_apply is None:
                place_island_props.os.environ.pop("PLACE_PROPS_APPLY", None)
            else:
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = original_apply
        self.assertEqual(level.mutations, [])
        self.assertIsNone(duplicate_a.location)
        self.assertIsNone(duplicate_b.location)

    def test_preflight_failure_leaves_managed_actors_untouched(self):
        engine, level = fake_unreal(None)
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
        original_apply = place_island_props.os.environ.get("PLACE_PROPS_APPLY")
        try:
            place_island_props.unreal = engine
            with tempfile.TemporaryDirectory() as temp_dir:
                config_path = Path(temp_dir) / "props.json"
                config_path.write_text(json.dumps({"props": [{
                    "id": "RatHollow",
                    "asset_path": "/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01",
                    "x": -105000,
                    "y": 104750,
                }]}), encoding="utf-8")
                place_island_props.CONFIG = str(config_path)
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = "1"
                with self.assertRaisesRegex(RuntimeError, "managed actors were left untouched"):
                    place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            if original_apply is None:
                place_island_props.os.environ.pop("PLACE_PROPS_APPLY", None)
            else:
                place_island_props.os.environ["PLACE_PROPS_APPLY"] = original_apply
        self.assertEqual(level.mutations, [])


if __name__ == "__main__":
    unittest.main()
