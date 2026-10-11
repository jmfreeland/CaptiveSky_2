#!/usr/bin/env python3
"""Review resident proposals without executing their text or launching asset tools."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INBOX = REPO_ROOT / "Saved" / "CaptiveSky" / "ComfyBlender" / "Requests" / "inbox.jsonl"
PIPELINE_ROOT = REPO_ROOT / "Scripts" / "AssetPipeline"
ASSET_SCRIPTS = PIPELINE_ROOT / "assets"
MAKE_SCRIPT = PIPELINE_ROOT / "make.py"
ASSET_SCRIPT_NAME = re.compile(r"^[a-z][a-z0-9_]{0,63}$")
FINAL_STATUSES = ("accepted", "generated", "declined")
ALL_STATUSES = ("pending_review",) + FINAL_STATUSES
STATUS_TRANSITIONS = {
    "pending_review": ("accepted", "declined"),
    "accepted": ("generated", "declined"),
    "generated": (),
    "declined": (),
}


class InboxError(Exception):
    """Raised when inbox contents cannot be safely reviewed or updated."""


def read_records(path: Path) -> list[dict]:
    try:
        content = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return []
    except (OSError, UnicodeError) as exc:
        raise InboxError(f"Cannot read inbox {path}: {exc}") from exc

    records = []
    decoder = json.JSONDecoder()
    offset = 0
    while offset < len(content):
        while offset < len(content) and content[offset].isspace():
            offset += 1
        if offset == len(content):
            break
        try:
            record, end = decoder.raw_decode(content, offset)
        except json.JSONDecodeError as exc:
            raise InboxError(f"Malformed inbox data at character {exc.pos}; no changes made.") from exc
        if not isinstance(record, dict):
            raise InboxError(f"Inbox record {len(records) + 1} is not a JSON object; no changes made.")
        request_id = record.get("id")
        if not isinstance(request_id, str) or not request_id.strip():
            raise InboxError(f"Inbox record {len(records) + 1} has no valid id; no changes made.")
        status = record.get("status", "pending_review")
        if status not in ALL_STATUSES:
            raise InboxError(f"Inbox record {request_id} has an invalid status; no changes made.")
        record["status"] = status
        records.append(record)
        offset = end

    ids = [record["id"] for record in records]
    if len(ids) != len(set(ids)):
        raise InboxError("Inbox contains duplicate request IDs; no changes made.")
    return records


def write_records(path: Path, records: list[dict]) -> None:
    temporary_path = None
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", newline="\n", dir=path.parent,
            prefix=f".{path.name}.", suffix=".tmp", delete=False,
        ) as temporary:
            temporary_path = Path(temporary.name)
            for record in records:
                temporary.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")))
                temporary.write("\n")
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_path, path)
    except (OSError, TypeError, ValueError) as exc:
        if temporary_path is not None:
            try:
                temporary_path.unlink(missing_ok=True)
            except OSError:
                pass
        raise InboxError(f"Cannot safely update inbox {path}: {exc}") from exc


def print_record(record: dict) -> None:
    def safe_text(value: object) -> str:
        return "".join(character if character.isprintable() else " " for character in str(value))

    request_type = record.get("request_type", "new_object")
    category = record.get("upgrade_kind")
    kind = f"{request_type}/{category}" if category else request_type
    requester = record.get("requester", "unknown resident")
    print(f"{safe_text(record['id'])} [{safe_text(record['status'])}] {safe_text(requester)} — {safe_text(kind)}")
    if record.get("target"):
        print(f"  target: {safe_text(record['target'])}")
    print(f"  {safe_text(record.get('description', '(no description)'))}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inbox", type=Path, default=DEFAULT_INBOX,
                        help="proposal inbox path (defaults to this project's Saved directory)")
    commands = parser.add_subparsers(dest="command", required=True)
    list_command = commands.add_parser("list", help="list pending requests")
    list_command.add_argument("--all", action="store_true", help="include requests already handled")
    status_command = commands.add_parser("status", help="mark one reviewed request")
    status_command.add_argument("request_id", help="exact request ID from the list")
    status_command.add_argument("status", choices=FINAL_STATUSES,
                                help="accepted, generated, or declined; this does not run a pipeline")
    route_command = commands.add_parser(
        "route", help="choose human review or ComfyBlender for an accepted visual upgrade"
    )
    route_command.add_argument("request_id", help="exact accepted upgrade request ID from the list")
    route_command.add_argument("pipeline", choices=("ComfyBlender", "human_review"))
    build_command = commands.add_parser(
        "build", help="run one named, human-authored asset script for an accepted object or visual-upgrade request"
    )
    build_command.add_argument("request_id", help="exact accepted request ID from the list")
    build_command.add_argument("asset_script", help="script name under Scripts/AssetPipeline/assets, without .py")
    return parser


def run_accepted_build(records: list[dict], request_id: str, asset_script: str) -> int:
    matches = [record for record in records if record["id"] == request_id]
    if not matches:
        raise InboxError(f"Request ID {request_id!r} was not found; no pipeline was run.")
    record = matches[0]
    if record["status"] != "accepted":
        raise InboxError(f"Request {request_id!r} is {record['status']}; only accepted requests may run a pipeline.")
    request_type = record.get("request_type", "new_object")
    if record.get("pipeline", "ComfyBlender") != "ComfyBlender":
        raise InboxError("This request is not assigned to ComfyBlender; no pipeline was run.")
    if request_type == "upgrade":
        if record.get("upgrade_kind") not in ("aesthetic", "variation"):
            raise InboxError("Only aesthetic or variation upgrades may use ComfyBlender; no pipeline was run.")
    elif request_type != "new_object":
        raise InboxError("This proposal type is not supported by ComfyBlender; no pipeline was run.")
    if not ASSET_SCRIPT_NAME.fullmatch(asset_script):
        raise InboxError("Asset script must be a lowercase name under assets/; no pipeline was run.")

    assets_root = ASSET_SCRIPTS.resolve()
    try:
        source = (ASSET_SCRIPTS / f"{asset_script}.py").resolve(strict=True)
    except OSError as exc:
        raise InboxError(f"Asset script {asset_script!r} was not found; no pipeline was run.") from exc
    if not source.is_file() or assets_root not in source.parents:
        raise InboxError("Asset script must be a file directly inside the pipeline assets folder; no pipeline was run.")
    if not MAKE_SCRIPT.is_file():
        raise InboxError("The ComfyBlender make.py entry point is missing; no pipeline was run.")

    try:
        result = subprocess.run(
            [sys.executable, "-I", str(MAKE_SCRIPT), asset_script],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError as exc:
        raise InboxError(f"The ComfyBlender pipeline could not be started: {exc}") from exc
    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    if result.stderr:
        print(result.stderr, file=sys.stderr, end="" if result.stderr.endswith("\n") else "\n")
    build_marker = f"[{asset_script}] build rc=0 exported=True"
    if (result.returncode != 0 or build_marker not in result.stdout or
            "PROBLEMS []" not in result.stdout or "VERIFY DONE" not in result.stdout):
        print("Asset pipeline did not verify a clean export; request status remains accepted.", file=sys.stderr)
        return result.returncode or 1
    print("Pipeline export passed its verifier. Inspect the FBX and Preview.png before marking the request generated.")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        records = read_records(args.inbox)
        if args.command == "list":
            visible = records if args.all else [record for record in records if record["status"] == "pending_review"]
            if not visible:
                print("No matching resident proposals.")
                return 0
            for index, record in enumerate(visible):
                if index:
                    print()
                print_record(record)
            return 0

        if args.command == "build":
            return run_accepted_build(records, args.request_id, args.asset_script)

        if args.command == "route":
            matches = [record for record in records if record["id"] == args.request_id]
            if not matches:
                raise InboxError(f"Request ID {args.request_id!r} was not found; no changes made.")
            record = matches[0]
            if record["status"] != "accepted":
                raise InboxError("Only accepted requests may be routed; no changes made.")
            if record.get("request_type", "new_object") != "upgrade":
                raise InboxError("Only upgrade requests can be routed; new objects already use ComfyBlender.")
            if args.pipeline == "ComfyBlender" and record.get("upgrade_kind") not in ("aesthetic", "variation"):
                raise InboxError("Functionality upgrades stay on human review; no changes made.")
            record["pipeline"] = args.pipeline
            write_records(args.inbox, records)
            print(f"Routed {args.request_id} to {args.pipeline}. No generator or Unreal import was run.")
            return 0

        matches = [record for record in records if record["id"] == args.request_id]
        if not matches:
            raise InboxError(f"Request ID {args.request_id!r} was not found; no changes made.")
        record = matches[0]
        current_status = record["status"]
        if args.status not in STATUS_TRANSITIONS[current_status]:
            allowed = ", ".join(STATUS_TRANSITIONS[current_status]) or "no further transitions"
            raise InboxError(
                f"Request {args.request_id!r} is {current_status}; allowed next status: {allowed}. "
                "No changes made."
            )
        record["status"] = args.status
        write_records(args.inbox, records)
        print(f"Marked {args.request_id} as {args.status}. No generator or Unreal import was run.")
        return 0
    except InboxError as exc:
        print(f"Request review error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
