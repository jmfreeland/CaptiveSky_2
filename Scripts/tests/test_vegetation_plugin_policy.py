"""Keep the retired PlantFactory mount out of the active UE project."""

import json
from pathlib import Path
import unittest


PROJECT = Path(__file__).resolve().parents[2]


class VegetationPluginPolicyTests(unittest.TestCase):
    def test_pve_stays_available_without_plantfactory(self):
        descriptor = json.loads((PROJECT / "CaptiveSky_2.uproject").read_text(encoding="utf-8"))
        plugins = {entry["Name"]: entry for entry in descriptor.get("Plugins", [])}

        self.assertTrue(plugins.get("ProceduralVegetationEditor", {}).get("Enabled"))
        self.assertNotIn("PlantFactoryPlugin", plugins)
        self.assertFalse((PROJECT / "Plugins" / "PlantFactoryPlugin").exists())


if __name__ == "__main__":
    unittest.main()
