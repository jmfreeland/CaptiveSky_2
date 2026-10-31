#!/usr/bin/env python3
"""Readable summary of one Island play session, from its Unreal log.

Reports how long the session ran and how many model requests it used, what each resident decided
(by action), what they interacted with, which actions failed and why, and any lasting changes the
world recorded. Optionally links logged proposal IDs to full descriptions in the request inbox.
Read-only; prints Markdown (or writes it with --out).

    python Scripts/Summarize-Session.py Saved/Logs/LiveMorning_2026-09-28.log
    python Scripts/Summarize-Session.py path/to/session.log --out Saved/SessionReports/today.md
    python Scripts/Summarize-Session.py path/to/session.log --requests Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl
    python Scripts/Summarize-Session.py path/to/multi-session.log --session 1

Runs launched with -CaptiveSkyLogSituations also report prompt sizes and which lasting things
(trail, cairn, arranging grounds, nests, guest book, ...) each resident was actually shown.
For logs with multiple PIE runs, the latest bounded run is selected by default; use --session N
to inspect an earlier run. Logs without session markers are summarized whole.
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import json
import re
from pathlib import Path

# EAgentActionType order in AgentLLMTypes.h. Keep this in sync when adding a
# resident action; otherwise session reports silently collapse new agency into
# anonymous `action N` buckets.
ACTIONS = [
    "idle", "move_to", "speak", "wander", "interact", "sleep", "build",
    "land", "request_object", "request_upgrade", "practice_craft",
]
STAMP = re.compile(r"^\[(\d{4})\.(\d{2})\.(\d{2})-(\d{2})\.(\d{2})\.(\d{2}):(\d{3})\]")
DECIDED = re.compile(r"LogAutonomousAgentAI: (\S+) decided: \"(.*)\" \(Action=(\d+)\)")
OUTCOME = re.compile(r"LogAutonomousAgentAI: (\S+) outcome: (.*)$")
SPEECH = re.compile(r'LogAutonomousAgentAI: (\S+) chose speech: "(.*)"$')
FEEDBACK_INVITE = re.compile(r"LogAgentBrain: Optional resident-feedback invitation shown to (\S+) after (\d+) real seconds\.")
FEEDBACK_RESULT = re.compile(r'LogAgentBrain: Resident feedback check-in result for (\S+): (.*)$')
SITUATION = re.compile(r"Situation for (\S+) \((\d+) chars, system prompt (\d+) chars\): (.*)$")
PROPOSAL_ID = re.compile(r"(?:Object request|Upgrade suggestion) ([0-9a-fA-F]{8})\b")
SESSION_START = re.compile(r"LogAgentSession: (?:Play safety active:|Continuous play:)")
SESSION_END = re.compile(r"LogAgentSession: Play ended after ")
# Things a resident can be told about, and the phrase that shows it was in view.
NOTICEABLE = {
    "pale-stone trail": "pale stone", "seed pod": "pod", "cairn": "small cairn", "arranging ground": "arranging",
    "stone arrangement": "stones into a", "nest": "nest", "guest book": "guest book", "inn hearth": "hearth",
    "firefly": "firefly", "minnows": "minnow", "crab": "crab", "storm signs": "storm may be coming", "mist": "mist",
}
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
    if controller.startswith("Rat"):
        return "Rat"
    for agent_prefix, display_name in (
        ("Agent_Aster_", "Aster"), ("Agent_Innkeeper_", "Innkeeper"),
        ("Agent_Rat_", "Rat"), ("Agent_Raven_", "Raven"),
    ):
        if controller.startswith(agent_prefix):
            return display_name
    joined = " ".join(thoughts).lower()
    if "hearth" in joined or "the inn" in joined or "guest book" in joined:
        return f"{controller} (likely the innkeeper)"
    return controller


def select_session(lines: list[str], requested_session: int) -> tuple[list[str], int | None, int]:
    ranges: list[tuple[int, int]] = []
    active_start: int | None = None
    for index, line in enumerate(lines):
        if SESSION_START.search(line):
            if active_start is not None:
                ranges.append((active_start, index))
            active_start = index
        elif SESSION_END.search(line) and active_start is not None:
            ranges.append((active_start, index + 1))
            active_start = None
    if active_start is not None:
        ranges.append((active_start, len(lines)))
    if not ranges:
        return lines, None, 0

    if requested_session == -1:
        selected_index = len(ranges) - 1
    elif requested_session > 0:
        selected_index = requested_session - 1
    else:
        raise ValueError("Session number must be positive, or -1 for the latest session.")
    if selected_index >= len(ranges):
        raise ValueError(f"Session {requested_session} is not present; log contains {len(ranges)} session(s).")
    start, end = ranges[selected_index]
    return lines[start:end], selected_index + 1, len(ranges)


def summarize(path: Path, request_inbox: Path | None = None, session: int = -1) -> str:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    lines, selected_session, session_count = select_session(lines, session)
    stamps = [stamp for stamp in (timestamp(line) for line in lines) if stamp]
    decisions: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    thoughts: dict[str, list[str]] = collections.defaultdict(list)
    speech: dict[str, list[str]] = collections.defaultdict(list)
    feedback_invites: dict[str, int] = {}
    feedback_results: dict[str, str] = {}
    interactions: collections.Counter = collections.Counter()
    failures: collections.Counter = collections.Counter()
    builds: list[str] = []
    lasting: list[str] = []
    budget: list[str] = []
    proposal_prefixes: set[str] = set()
    situations: dict[str, list[int]] = collections.defaultdict(list)
    systems: dict[str, list[int]] = collections.defaultdict(list)
    noticed: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    for line in lines:
        if match := FEEDBACK_INVITE.search(line):
            agent, elapsed = match.groups()
            feedback_invites[agent] = int(elapsed)
            continue
        if match := FEEDBACK_RESULT.search(line):
            agent, result = match.groups()
            feedback_results[agent] = result
            continue
        if match := SITUATION.search(line):
            agent, length, system, text = match.groups()
            situations[agent].append(int(length))
            systems[agent].append(int(system))
            for thing, phrase in NOTICEABLE.items():
                if phrase in text: noticed[agent][thing] += 1
            continue
        if match := DECIDED.search(line):
            controller, thought, action = match.groups()
            index = int(action)
            decisions[controller][ACTIONS[index] if index < len(ACTIONS) else f"action {index}"] += 1
            thoughts[controller].append(thought)
            continue
        if match := SPEECH.search(line):
            controller, words = match.groups()
            speech[controller].append(" ".join(words.split()))
            continue
        if match := OUTCOME.search(line):
            controller, outcome = match.groups()
            if proposal_match := PROPOSAL_ID.search(outcome):
                proposal_prefixes.add(proposal_match.group(1).lower())
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

    session_label = f" (session {selected_session} of {session_count})" if selected_session is not None else ""
    out = [f"# Session summary: {path.name}{session_label}", ""]
    if stamps:
        out.append(f"- Ran {stamps[0]:%Y-%m-%d %H:%M} to {stamps[-1]:%H:%M} ({(stamps[-1] - stamps[0]).total_seconds() / 60:.0f} minutes of log).")
    out += [f"- {entry}" for entry in budget]
    total = sum(sum(counter.values()) for counter in decisions.values())
    out += ["", f"## Decisions ({total})", "", "| Resident | " + " | ".join(ACTIONS) + " | total |", "|---" * (len(ACTIONS) + 2) + "|"]
    for controller, counter in sorted(decisions.items()):
        out.append(f"| {resident_name(controller, thoughts[controller])} | " + " | ".join(str(counter[action]) for action in ACTIONS) + f" | {sum(counter.values())} |")
    out += ["", "## What residents said", ""]
    for controller, lines_spoken in sorted(speech.items()):
        out.extend(f"- {resident_name(controller, thoughts[controller])}: “{words}”" for words in lines_spoken)
    if not speech:
        out.append("- No speech lines logged.")
    if feedback_invites:
        out += ["", "## Optional resident feedback", ""]
        for agent, elapsed in sorted(feedback_invites.items()):
            result = feedback_results.get(agent, "invitation was shown; no result was logged")
            out.append(f"- {resident_name(agent, thoughts[agent])} (invited after {elapsed // 60} min): {result}")
    if situations:
        out += ["", "## What residents were shown", "", "| Resident | situations | median situation chars | median system prompt chars | most often in view |", "|---|---|---|---|---|"]
        for agent, lengths in sorted(situations.items()):
            ordered, prompts = sorted(lengths), sorted(systems[agent])
            seen = ", ".join(f"{thing} {count}/{len(lengths)}" for thing, count in noticed[agent].most_common(6)) or "none of the tracked things"
            out.append(f"| {agent} | {len(lengths)} | {ordered[len(ordered) // 2]} | {prompts[len(prompts) // 2]} | {seen} |")
    out += ["", "## What they interacted with", ""]
    out += [f"- {target}: {count}" for target, count in interactions.most_common()] or ["- nothing"]
    out += ["", "## Things made or changed by residents", ""]
    out += [f"- {entry}" for entry in builds] or ["- nothing"]
    out += ["", "## Failed or refused actions", ""]
    out += [f"- ×{count} {outcome}" for outcome, count in failures.most_common()] or ["- none"]
    if request_inbox is not None:
        matched_proposals = []
        if request_inbox.is_file() and proposal_prefixes:
            for line in request_inbox.read_text(encoding="utf-8", errors="replace").splitlines():
                try:
                    proposal = json.loads(line)
                except json.JSONDecodeError:
                    continue
                proposal_id = str(proposal.get("id", "")).replace("-", "").lower()
                if any(proposal_id.startswith(prefix) for prefix in proposal_prefixes):
                    matched_proposals.append(proposal)
        out += ["", "## Resident proposals", ""]
        for proposal in matched_proposals:
            request_type = str(proposal.get("request_type", "new_object"))
            kind = str(proposal.get("upgrade_kind", ""))
            target = str(proposal.get("target", ""))
            requester = str(proposal.get("requester", "unknown resident"))
            description = " ".join(str(proposal.get("description", "")).split())
            status = str(proposal.get("status", "unknown status"))
            details = f" ({kind})" if kind else ""
            target_text = f" for `{target}`" if target else ""
            out.append(f"- {request_type}{details}{target_text} by `{requester}`: {description} — {status}")
        if not matched_proposals:
            out.append("- No inbox records matched the request IDs logged in this session.")
    out += ["", "## World-state log", ""]
    out += [f"- {entry}" for entry in lasting] or ["- nothing logged"]
    return "\n".join(out) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--requests", type=Path, help="Optional resident-proposal inbox JSONL to enrich logged request IDs.")
    parser.add_argument("--session", type=int, default=-1,
                        help="1-based session within a multi-session log; defaults to the latest. Logs without session markers are summarized whole.")
    args = parser.parse_args()
    report = summarize(args.log, args.requests, args.session)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(report, encoding="utf-8")
    print(report)


if __name__ == "__main__":
    main()
