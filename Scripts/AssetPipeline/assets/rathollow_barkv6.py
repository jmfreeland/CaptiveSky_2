"""RatHollow V6 tests a quieter generated bark albedo on the V5 shell."""

import importlib.util
import hashlib
import os
from pathlib import Path
import shutil


_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow.py")
_SPEC = importlib.util.spec_from_file_location("rathollow_barkv6_base", _SOURCE)
rathollow = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(rathollow)

NAME = "RatHollow_BarkV6"
TEXTURE_FILES = (
    "T_Lib_RatHollow_BarkV6_BC.png",
    "T_Lib_RatHollow_BarkV6_N.png",
    "T_Lib_RatHollow_BarkV6_ORM.png",
)


def _ensure_versioned_textures():
    source_dir = Path(__file__).with_name("textures") / "RatHollow_BarkV6"
    library_dir = Path(rathollow.lib.ROOT) / "Library" / "Textures"
    library_dir.mkdir(parents=True, exist_ok=True)
    for filename in TEXTURE_FILES:
        source = source_dir / filename
        destination = library_dir / filename
        if not source.is_file():
            raise FileNotFoundError("RatHollow V6 texture source is missing: {}".format(source))
        if destination.is_file():
            source_hash = hashlib.sha256(source.read_bytes()).digest()
            destination_hash = hashlib.sha256(destination.read_bytes()).digest()
            if source_hash != destination_hash:
                raise RuntimeError(
                    "Refusing to overwrite a different local RatHollow V6 texture: {}".format(destination)
                )
            continue
        shutil.copy2(str(source), str(destination))


def build(A):
    _ensure_versioned_textures()
    # The shared Bark texture remains unchanged.
    rathollow.lib.TILED["bark"] = ("T_Lib_RatHollow_BarkV6", 1.2)
    rathollow._hollow_log(A, conform_moss=True)
    rathollow._triangulate_export_meshes(A)
