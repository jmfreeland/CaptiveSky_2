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
    def __init__(self):
        self.mutations = []

    def load_level(self, _path):
        pass

    def get_editor_world(self):
        return object()

    def get_all_level_actors(self):
        raise AssertionError("preflight and dry-run must not inspect or delete actors")

    def destroy_actor(self, actor):
        self.mutations.append(("destroy", actor))

    def spawn_actor_from_class(self, *args):
        self.mutations.append(("spawn", args))

    def save_current_level(self):
        self.mutations.append(("save",))


def fake_unreal(mesh):
    level = FakeEditorLevelLibrary()
    engine = types.SimpleNamespace(
        Paths=types.SimpleNamespace(project_dir=lambda: ""),
        EditorLevelLibrary=level,
        SystemLibrary=types.SimpleNamespace(line_trace_single=lambda *args: FakeHit()),
        TraceTypeQuery=types.SimpleNamespace(TRACE_TYPE_QUERY1=1),
        DrawDebugTrace=types.SimpleNamespace(NONE=0),
        Vector=FakeVector,
        Actor=type("Actor", (), {}),
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

    def test_dry_run_does_not_destroy_spawn_or_save_actors(self):
        engine, level = fake_unreal(FakeMesh())
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
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
                place_island_props.os.environ["PLACE_PROPS_DRYRUN"] = "1"
                place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
            if original_dry_run is None:
                place_island_props.os.environ.pop("PLACE_PROPS_DRYRUN", None)
            else:
                place_island_props.os.environ["PLACE_PROPS_DRYRUN"] = original_dry_run
        self.assertEqual(level.mutations, [])

    def test_preflight_failure_leaves_managed_actors_untouched(self):
        engine, level = fake_unreal(None)
        original_engine = place_island_props.unreal
        original_config = place_island_props.CONFIG
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
                with self.assertRaisesRegex(RuntimeError, "managed actors were left untouched"):
                    place_island_props.run()
        finally:
            place_island_props.unreal = original_engine
            place_island_props.CONFIG = original_config
        self.assertEqual(level.mutations, [])


if __name__ == "__main__":
    unittest.main()
