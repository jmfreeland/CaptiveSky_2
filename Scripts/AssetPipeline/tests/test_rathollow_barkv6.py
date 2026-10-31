"""Versioned texture-pack checks for the experimental RatHollow V6 builder."""

from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
TEXTURE_DIR = ASSET_DIR / "textures" / "RatHollow_BarkV6"
BUILDER = ASSET_DIR / "rathollow_barkv6.py"


class RatHollowBarkV6Tests(unittest.TestCase):
    def test_builder_uses_its_versioned_complete_texture_pack(self):
        for suffix in ("BC", "N", "ORM"):
            with self.subTest(suffix=suffix):
                self.assertTrue((TEXTURE_DIR / f"T_Lib_RatHollow_BarkV6_{suffix}.png").is_file())
        source = BUILDER.read_text(encoding="utf-8")
        self.assertIn('NAME = "RatHollow_BarkV6"', source)
        self.assertIn('"T_Lib_RatHollow_BarkV6", 1.2', source)
        self.assertIn("Refusing to overwrite a different local RatHollow V6 texture", source)


if __name__ == "__main__":
    unittest.main()
