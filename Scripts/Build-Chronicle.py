#!/usr/bin/env python3
"""Readable account of what has happened on the Island, from WorldState/chronicle.jsonl.

The game appends one JSON line per event (residents' decisions, things made or changed, weather, days
passing). This groups them by Island day and tells them in order: speech and lasting changes are
listed one by one, quiet walking about is folded into a single line per resident. Read-only; prints
Markdown, or writes Markdown / a standalone HTML page.

    python Scripts/Build-Chronicle.py
    python Scripts/Build-Chronicle.py --day 4 --out Saved/Chronicle/day4.md
    python Scripts/Build-Chronicle.py --html Saved/Chronicle/island.html
"""
from __future__ import annotations

import argparse
import collections
import html
import json
import re
import sys
from pathlib import Path

DEFAULT_LOG = Path(__file__).resolve().parent.parent / "WorldState" / "chronicle.jsonl"
QUIET = {"idle": "rested", "wander": "wandered", "move_to": "went somewhere", "sleep": "slept"}
LASTING = {"nest", "curio", "arrangement", "arrangement_response", "guest_book", "storm_mark"}


def load(path: Path) -> list[dict]:
    entries = []
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        try:
            entry = json.loads(line)
        except json.JSONDecodeError:
            print(f"skipping unreadable line {number}")
            continue
        if isinstance(entry, dict) and "type" in entry:
            entries.append(entry)
    return entries


def name(agent: str) -> str:
    """Agent_Aster_01 -> Aster; Agent_Raven_01 -> Raven; anything else as written."""
    if not agent:
        return "Someone"
    match = re.fullmatch(r"Agent_(.+?)(?:_\d+)?", agent)
    return (match.group(1) if match else agent).replace("_", " ")


def quote(text: str) -> str:
    return '"' + text.strip().replace('"', "'") + '"'


def describe(entry: dict) -> str | None:
    kind, who, text = entry["type"], name(entry.get("agent", "")), (entry.get("text") or "").strip()
    if kind == "decision":
        action, target = entry.get("action", "idle"), entry.get("target", "")
        if action == "speak":
            partner = entry.get("with")
            return f"{who} said{' to ' + partner if partner else ''}: {quote(text)}"
        if action == "interact":
            return f"{who} reached for {target or 'something nearby'}" + (f" — {text}" if text else "")
        if action == "build":
            return f"{who} set to work at {target or 'a place offered to them'}" + (f" — {text}" if text else "")
        return None
    if kind == "guest_book":
        return f"{who} wrote in the guest book: {quote(text)}"
    if kind in LASTING:
        return f"{who} {text}" if entry.get("agent") else text[:1].upper() + text[1:]
    if kind in ("weather", "day"):
        return text
    return None


def fold_quiet(entries: list[dict]) -> list[str]:
    """One line per resident summarizing their unremarkable turns."""
    counts: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    for entry in entries:
        if entry["type"] == "decision" and entry.get("action") in QUIET:
            counts[name(entry.get("agent", ""))][entry["action"]] += 1
    lines = []
    for who, tally in sorted(counts.items()):
        parts = [f"{QUIET[action]} {n}×" for action, n in tally.most_common()]
        lines.append(f"Otherwise {who} {', '.join(parts)}.")
    return lines


def render(entries: list[dict], only_day: int | None) -> str:
    by_day: dict[int, list[dict]] = collections.defaultdict(list)
    for entry in entries:
        by_day[int(entry.get("day", 0))].append(entry)
    out = ["# The Island chronicle", ""]
    shown = 0
    for day in sorted(by_day):
        if only_day is not None and day != only_day:
            continue
        events = by_day[day]
        lines = [f"**{e.get('clock', '--:--')}** {text}" for e in events if (text := describe(e))]
        sessions = sum(1 for e in events if e["type"] == "session" and e.get("event") == "start")
        speakers = sorted({name(e.get("agent", "")) for e in events if e["type"] == "decision"})
        out += [f"## Day {day}", ""]
        if speakers:
            out += [f"*Residents about: {', '.join(speakers)}" + (f" · {sessions} session(s) opened" if sessions else "") + "*", ""]
        out += [f"- {line}" for line in lines] or ["- Nothing of note was recorded."]
        quiet = fold_quiet(events)
        if quiet:
            out += ["", *[f"> {line}" for line in quiet]]
        out.append("")
        shown += 1
    if not shown:
        out.append("Nothing has been recorded yet." if only_day is None else f"Nothing was recorded on day {only_day}.")
    return "\n".join(out).rstrip() + "\n"


PAGE = """<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>The Island chronicle</title>
<style>body{{max-width:44rem;margin:2rem auto;padding:0 1rem;font:16px/1.55 Georgia,serif;color:#1d2a2c;background:#f4f7f6}}
h1{{font-weight:400}}h2{{margin-top:2.2rem;border-bottom:1px solid #b9c9c6}}ul{{padding-left:1.1rem}}li{{margin:.3rem 0}}
blockquote{{margin:.8rem 0;color:#5b6b6d;font-style:italic;border-left:3px solid #b9c9c6;padding-left:.8rem}}
@media(prefers-color-scheme:dark){{body{{color:#dbe6e4;background:#141b1c}}h2{{border-color:#33423f}}blockquote{{color:#93a5a3;border-color:#33423f}}}}</style>
{body}"""


def to_html(markdown: str) -> str:
    body, in_list = [], False
    for line in markdown.splitlines():
        text = html.escape(line)
        text = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", text)
        text = re.sub(r"\*(.+?)\*", r"<i>\1</i>", text)
        if line.startswith("- "):
            body.append(("" if in_list else "<ul>") + f"<li>{text[2:]}</li>")
            in_list = True
            continue
        if in_list:
            body.append("</ul>")
            in_list = False
        if line.startswith("# "):
            body.append(f"<h1>{text[2:]}</h1>")
        elif line.startswith("## "):
            body.append(f"<h2>{text[3:]}</h2>")
        elif line.startswith("&gt; "):
            body.append(f"<blockquote>{text[5:]}</blockquote>")
        elif text.strip():
            body.append(f"<p>{text}</p>")
    if in_list:
        body.append("</ul>")
    return PAGE.format(body="\n".join(body))


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", nargs="?", type=Path, default=DEFAULT_LOG, help="chronicle.jsonl (default: WorldState/chronicle.jsonl)")
    parser.add_argument("--day", type=int, help="only this Island day")
    parser.add_argument("--out", type=Path, help="write Markdown here instead of printing")
    parser.add_argument("--html", type=Path, help="also write a standalone HTML page here")
    args = parser.parse_args()
    if not args.log.exists():
        raise SystemExit(f"No chronicle at {args.log}. It is written while the game runs.")
    markdown = render(load(args.log), args.day)
    if args.html:
        args.html.parent.mkdir(parents=True, exist_ok=True)
        args.html.write_text(to_html(markdown), encoding="utf-8")
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(markdown, encoding="utf-8")
    elif not args.html:
        print(markdown, end="")


if __name__ == "__main__":
    main()
