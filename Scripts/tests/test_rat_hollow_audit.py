import importlib.util
from pathlib import Path
import sys
import types
import unittest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "Inspect-IslandRatHollowCandidates.py"
sys.modules.setdefault("unreal", types.ModuleType("unreal"))
SPEC = importlib.util.spec_from_file_location("rat_hollow_audit", SCRIPT_PATH)
rat_hollow_audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(rat_hollow_audit)


class RatHollowCandidateTests(unittest.TestCase):
    def test_explicit_hollow_tag_is_authoritative_shortlist_even_without_mesh_hint(self):
        self.assertEqual(rat_hollow_audit.candidate_kind(
            "BurrowMarker", "Actor_1", [], {"RatHollow", "IslandLandmark"}), "tagged")

    def test_fallen_log_mesh_name_is_only_a_hint(self):
        self.assertEqual(rat_hollow_audit.candidate_kind(
            "StaticMeshActor_17", "StaticMeshActor_17", ["/Game/Forest/SM_Fallen_Log"], set()), "hint")

    def test_driftwood_bench_and_firewood_stack_are_not_hollows(self):
        self.assertIsNone(rat_hollow_audit.candidate_kind(
            "TideglassBench", "Prop_TideglassBench", ["/Game/Props/DriftwoodBench"], set()))
        self.assertIsNone(rat_hollow_audit.candidate_kind(
            "InnFirewood", "Prop_InnFirewood", ["/Game/Props/FirewoodStack"], set()))

    def test_unrelated_mesh_and_non_mesh_actor_are_ignored(self):
        self.assertIsNone(rat_hollow_audit.candidate_kind(
            "StandingStone", "ListeningStone_C", ["/Game/Props/Stone"], {"IslandLandmark"}))
        self.assertIsNone(rat_hollow_audit.candidate_kind(
            "EmptyMarker", "Actor_2", [], set()))


if __name__ == "__main__":
    unittest.main()
