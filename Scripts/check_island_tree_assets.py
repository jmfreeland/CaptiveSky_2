"""Preflight Megaplant tree packages referenced by IslandWeather.cpp.

This only checks whether the expected .uasset files exist on disk. It does not
load them through Unreal or validate materials, wind, Nanite, or runtime scatter.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path, PurePosixPath


TREE_ASSET_PREFIX = "/Game/Megaplant_Library/"
TREE_REFERENCE_PATTERN = re.compile(r'TEXT\("(?P<asset>/Game/Megaplant_Library/[^"\\]+)"\)')
SOURCE_RELATIVE_PATH = Path("Source/CaptiveSky_2/Agent/IslandWeather.cpp")


def referenced_tree_packages(source_text: str) -> list[str]:
    """Return unique Unreal object paths for referenced Megaplant packages."""
    return sorted(set(TREE_REFERENCE_PATTERN.findall(source_text)))


def package_file_for_reference(project_root: Path, object_path: str) -> Path:
    """Translate an Unreal /Game/Package.Object reference to its .uasset path."""
    if not object_path.startswith(TREE_ASSET_PREFIX):
        raise ValueError(f"Not a Megaplant library reference: {object_path}")
    package_path, separator, _object_name = object_path.rpartition(".")
    if not separator:
        package_path = object_path
    relative = PurePosixPath(package_path.removeprefix("/Game/"))
    return project_root / "Content" / Path(*relative.parts).with_suffix(".uasset")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="CaptiveSky project root (defaults to this script's parent project)",
    )
    args = parser.parse_args(argv)
    project_root = args.project_root.resolve()
    source_path = project_root / SOURCE_RELATIVE_PATH
    if not source_path.is_file():
        print(f"IslandWeather source not found: {source_path}", file=sys.stderr)
        return 2

    references = referenced_tree_packages(source_path.read_text(encoding="utf-8"))
    if not references:
        print(f"No {TREE_ASSET_PREFIX} references found in {source_path}", file=sys.stderr)
        return 2

    missing: list[Path] = []
    for reference in references:
        package_file = package_file_for_reference(project_root, reference)
        state = "OK" if package_file.is_file() else "MISSING"
        print(f"{state}: {reference} -> {package_file}")
        if not package_file.is_file():
            missing.append(package_file)

    print(f"Checked {len(references)} tree package(s); {len(missing)} missing.")
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
