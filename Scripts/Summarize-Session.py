#!/usr/bin/env python3
"""Readable summary of one Island play session, from its Unreal log.

Reports how long the session ran and how many model requests it used, what each resident decided
(by action), what they interacted with, which actions failed and why, and any lasting changes the
world recorded. Read-only; prints Markdown (or writes it with --out).

    python Scripts/Summarize-Session.py Saved/Logs/LiveMorning_2026-09-28.log
    python Scripts/Summarize-Session.py path/to/session.log --out Saved/SessionReports/today.md
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import re
from pathlib import Path

# EAgentActionType order in AgentLLMTypes.h.
ACTIONS = ["idle", "move_to", "speak", "wander", "interact", "sleep", "build"]
STAMP = re.compile(r"^\[(\d{4})\.(\d{2})\.(\d{2})-(\d{2})\.(\d{2})\.(\d{2}):(\d{3})\]")
DECIDED = re.compile(r"LogAutonomousAgentAI: (\S+) decided: \"(.*)\" \(Action=(\d+)\)")
OUTCOME = re.compile(r"LogAutonomousAgentAI: (\S+) outcome: (.*)$")
FAILURE_MARKERS = ("failed", "did not complete", "blocked", "not found", "cannot", "Cannot", "refused", "too far", "Nothing changed", "rejected")


def timestamp(line: str) -> dt.datetime | None:
    match = STAMP.match(line)
    if not match:
        return None
    year, month, day, hour, minute, second, milli = (int(part) for part in match.groups())
    return dt.datetime(year, month, day, hour, minute, second, milli * 1000)


def resident_name(controller: str, thoughts: list[str]) -> str:
    if controller.startswith("Raven"):
        return "Raven"
    joined = " ".join(thoughts).lower()
    if "hearth" in joined or "the inn" in joined or "guest book" in joined:
        return f"{controller} (likely the innkeeper)"
    return controller


def summarize(path: Path) -> str:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    stamps = [stamp for stamp in (timestamp(line) for line in lines) if stamp]
    decisions: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    thoughts: dict[str, list[str]] = collections.defaultdict(list)
    interactions: collections.Counter = collections.Counter()
    failures: collections.Counter = collections.Counter()
    builds: list[str] = []
    lasting: list[str] = []
    budget: list[str] = []
    for line in lines:
        if match := DECIDED.search(line):
            controller, thought, action = match.groups()
            index = int(action)
            decisions[controller][ACTIONS[index] if index < len(ACTIONS) else f"action {index}"] += 1
            thoughts[controller].append(thought)
            continue
        if match := OUTCOME.search(line):
            controller, outcome = match.groups()
            target = outcome.split(":", 1)[0] if ":" in outcome[:40] else ""
            if any(marker in outcome for marker in FAILURE_MARKERS) and not outcome.startswith(("Waiting quietly", "Settled")):
                failures[outcome[:110]] += 1
            elif target:
                interactions[target] += 1
            if any(word in outcome for word in ("wove", "gathered", "picked up a small bundle", "arranged", "set it on the small cairn", "husk-leaves", "signed")):
                builds.append(f"{controller}: {outcome[:160]}")
            continue
        if "LogIslandWorldState" in line or "LogIslandWeatherTraces" in line:
            lasting.append(re.sub(r"^\[[^\]]*\]\[[^\]]*\]", "", line).strip()[:200])
        elif "LogAgentSession" in line:
            budget.append(re.sub(r"^\[[^\]]*\]\[[^\]]*\]", "", line).strip()[:200])

    out = [f"# Session summary: {path.name}", ""]
    if stamps:
        out.append(f"- Ran {stamps[0]:%Y-%m-%d %H:%M} to {stamps[-1]:%H:%M} ({(stamps[-1] - stamps[0]).total_seconds() / 60:.0f} minutes of log).")
    out += [f"- {entry}" for entry in budget]
    total = sum(sum(counter.values()) for counter in decisions.values())
    out += ["", f"## Decisions ({total})", "", "| Resident | " + " | ".join(ACTIONS) + " | total |", "|---" * (len(ACTIONS) + 2) + "|"]
    for controller, counter in sorted(decisions.items()):
        out.append(f"| {resident_name(controller, thoughts[controller])} | " + " | ".join(str(counter[action]) for action in ACTIONS) + f" | {sum(counter.values())} |")
    out += ["", "## What they interacted with", ""]
    out += [f"- {target}: {count}" for target, count in interactions.most_common()] or ["- nothing"]
    out += ["", "## Things made or changed by residents", ""]
    out += [f"- {entry}" for entry in builds] or ["- nothing"]
    out += ["", "## Failed or refused actions", ""]
    out += [f"- ×{count} {outcome}" for outcome, count in failures.most_common()] or ["- none"]
    out += ["", "## World-state log", ""]
    out += [f"- {entry}" for entry in lasting] or ["- nothing logged"]
    return "\n".join(out) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    report = summarize(args.log)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(report, encoding="utf-8")
    print(report)


if __name__ == "__main__":
    main()
