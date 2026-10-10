#!/usr/bin/env python3
"""Review resident proposals without executing their text or launching asset tools."""

import argparse
import json
import os
from pathlib import Path
import sys
import tempfile


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INBOX = REPO_ROOT / "Saved" / "CaptiveSky" / "ComfyBlender" / "Requests" / "inbox.jsonl"
FINAL_STATUSES = ("accepted", "generated", "declined")
ALL_STATUSES = ("pending_review",) + FINAL_STATUSES


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
    return parser


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

        matches = [record for record in records if record["id"] == args.request_id]
        if not matches:
            raise InboxError(f"Request ID {args.request_id!r} was not found; no changes made.")
        matches[0]["status"] = args.status
        write_records(args.inbox, records)
        print(f"Marked {args.request_id} as {args.status}. No generator or Unreal import was run.")
        return 0
    except InboxError as exc:
        print(f"Request review error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
