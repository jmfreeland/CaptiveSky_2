"""Read-only comparison of tracked runtime sources with another project checkout.

This is useful when Unreal is open from a separate project directory and a
feature appears absent even though its source has been committed elsewhere.
It never copies, edits, or saves files.
"""
import argparse
import hashlib
from pathlib import Path, PurePosixPath, PureWindowsPath
import subprocess
import sys


RUNTIME_ROOTS = ("Source", "Config", "Scripts")
SOURCE_SUFFIXES = {".cpp", ".h", ".cs", ".ini", ".uproject", ".py", ".ps1"}


def tracked_runtime_sources(worktree_root):
    """Return tracked runtime/config/script paths without querying the live checkout."""
    result = subprocess.run(
        ["git", "-C", str(worktree_root), "ls-files", "-z", "--", *RUNTIME_ROOTS],
        check=True,
        capture_output=True,
    )
    paths = (item.decode("utf-8") for item in result.stdout.split(b"\0") if item)
    return sorted(path for path in paths if Path(path).suffix.lower() in SOURCE_SUFFIXES)


def _safe_relative_path(value):
    raw = str(value)
    path = PurePosixPath(raw.replace("\\", "/"))
    windows_path = PureWindowsPath(raw)
    if (path.is_absolute() or windows_path.is_absolute() or windows_path.drive
            or not path.parts or ".." in path.parts):
        raise ValueError("comparison paths must stay inside the project root: {}".format(value))
    return path


def _digest(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def compare_paths(worktree_root, live_root, relative_paths):
    """Classify each path as identical, different, source-missing, or live-missing."""
    worktree_root = Path(worktree_root)
    live_root = Path(live_root)
    resolved_roots = (worktree_root.resolve(), live_root.resolve())
    rows = []
    for value in relative_paths:
        relative = _safe_relative_path(value)
        candidates = tuple(root.joinpath(*relative.parts).resolve() for root in resolved_roots)
        for root, candidate in zip(resolved_roots, candidates):
            try:
                candidate.relative_to(root)
            except ValueError:
                raise ValueError("comparison path resolves outside the project root: {}".format(value))
        source, live = candidates
        if not source.is_file():
            status = "source-missing"
        elif not live.is_file():
            status = "live-missing"
        else:
            status = "identical" if _digest(source) == _digest(live) else "different"
        rows.append((status, relative.as_posix()))
    return rows


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("live_project_root", type=Path,
                        help="read-only root of the Unreal project currently in use")
    parser.add_argument("paths", nargs="*",
                        help="optional project-relative paths; default compares tracked runtime sources")
    args = parser.parse_args(argv)
    worktree_root = Path(__file__).resolve().parents[1]
    if not args.live_project_root.is_dir():
        parser.error("live project root does not exist: {}".format(args.live_project_root))

    try:
        paths = args.paths or tracked_runtime_sources(worktree_root)
        rows = compare_paths(worktree_root, args.live_project_root, paths)
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print("Parity audit failed: {}".format(exc), file=sys.stderr)
        return 2

    counts = {}
    for status, relative in rows:
        counts[status] = counts.get(status, 0) + 1
        if status != "identical":
            print("{}  {}".format(status.upper(), relative))
    print("Compared {} files: {} identical, {} different, {} live-missing, {} source-missing.".format(
        len(rows), counts.get("identical", 0), counts.get("different", 0),
        counts.get("live-missing", 0), counts.get("source-missing", 0)))
    return 1 if any(status != "identical" for status, _relative in rows) else 0


if __name__ == "__main__":
    raise SystemExit(main())
