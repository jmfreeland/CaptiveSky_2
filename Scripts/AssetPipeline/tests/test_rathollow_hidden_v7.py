"""Source-level contract checks for the unplaced hidden RatHollow V7 candidate."""

from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
BUILDER = ASSET_DIR / "rathollow_hidden_v7.py"


class RatHollowHiddenV7Tests(unittest.TestCase):
    def test_hidden_candidate_reuses_versioned_bark_and_keeps_named_anchors(self):
        source = BUILDER.read_text(encoding="utf-8")
        self.assertIn('NAME = "RatHollow_HiddenV7"', source)
        self.assertIn('"T_Lib_RatHollow_BarkV6", 1.2', source)
        self.assertIn('"entrance": (-132.0, 0.0, 66.0)', source)
        self.assertIn('"nest_bed": (-32.0, 0.0, 32.0)', source)
        self.assertIn('name="brokenRootCanopy"', source)
        self.assertIn('name="groundedRootNear"', source)
        self.assertIn('name="groundedRootFar"', source)
        self.assertIn("rathollow._hollow_log(A, conform_moss=True)", source)
        self.assertNotIn("rathollow._hollow_log(A, conform_moss=False)", source)


if __name__ == "__main__":
    unittest.main()
