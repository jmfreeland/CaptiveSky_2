#!/usr/bin/env python3
"""Read-only review of residents' long-term memory (Agents/<AgentId>/memory.jsonl).

Reports what each resident's memory is made of, how much of it is near-duplicate
reflection, and which memories retrieval would put in front of the model for a typical
situation. The simulation mirrors UAgentMemoryComponent::GetRelevantContext / ScoreRecord
(24 h recency half-life, 0.4 recency + 0.4 importance + 0.2 keyword overlap, Jaccard
near-duplicate filtering, dialogue share, and the 800-token ~ 3200-char budget); keep it
in step if those change.

Nothing is modified. The report can quote private memories, so it is written under the
git-ignored Saved/ directory by default.

    python Scripts/Analyze-AgentMemory.py
    python Scripts/Analyze-AgentMemory.py --agent Agent_Raven_01 --situation "rain at the pool"
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HALF_LIFE_HOURS = 24.0
CHAR_BUDGET = 800 * 4
SIMILAR = 0.6  # word-set Jaccard at or above which two memories count as near-duplicates

# A representative autonomous-think situation, in the shape BuildSituationSummary produces.
DEFAULT_SITUATION = (
    "You are at position (-99600, 100400, 2890). Nearby: Agent_Raven_01 is 3 metres away "
    "(move_to target: ApproachAgent_Agent_Raven_01). You may approach them, but moving closer does not "
    "begin a conversation; speaking remains optional for both of you. Last physical action result: "
    "Waiting quietly. No new action or discovery occurred. It is afternoon on the Island (approximately 15:20). "
    "The sun and moon move as time passes; cloud cover gently softens direct and ambient daylight. "
    "Cloud cover is moderate and a light breeze is blowing. The TideglassPool is 4 metres away "
    "(move_to/interact target: TideglassPool). The ListeningStones are 25 metres away (move_to/interact "
    "target: ListeningStones). no one is speaking to you right now. Decide what to do."
)


def tokens(text: str) -> list[str]:
    """Mirror of TokenizeLower: lower-case, non-alphanumerics become spaces, split on whitespace."""
    return re.sub(r"[^0-9a-z]", " ", text.lower()).split()


def parse_time(stamp: str) -> dt.datetime:
    return dt.datetime.fromisoformat(stamp.replace("Z", "+00:00"))


def load(agent: str) -> list[dict]:
    path = ROOT / "Agents" / agent / "memory.jsonl"
    records = []
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line:
                try:
                    records.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return records


def score(record: dict, situation: list[str], now: dt.datetime) -> float:
    age_hours = max((now - parse_time(record["timestamp"])).total_seconds() / 3600.0, 0.0)
    recency = math.exp(-age_hours / HALF_LIFE_HOURS)
    words = set(tokens(record.get("text", ""))) | {tag.lower() for tag in record.get("tags", [])}
    overlap = sum(1 for word in situation if word in words) / len(situation) if situation else 0.0
    return 0.4 * recency + 0.4 * float(record.get("importance", 0.5)) + 0.2 * overlap


def retrieve(records: list[dict], situation: str, now: dt.datetime) -> list[tuple[float, dict]]:
    words = tokens(situation)
    ranked = sorted(((score(r, words, now), r) for r in records), key=lambda pair: pair[0], reverse=True)
    non_conversation = [item for item in ranked if item[1].get("type") != "conversation"]
    conversations = [item for item in ranked if item[1].get("type") == "conversation"]
    chosen: list[tuple[float, dict]] = []
    chosen_words: list[set[str]] = []
    used = non_conversation_count = conversation_count = 0
    non_conversation_budget = CHAR_BUDGET - CHAR_BUDGET // 3

    def try_add(item: tuple[float, dict], character_limit: int) -> bool:
        nonlocal used
        value, record = item
        candidate_words = set(tokens(record.get("text", "")))
        if candidate_words and any(
            len(candidate_words & existing) / len(candidate_words | existing) >= SIMILAR
            for existing in chosen_words if existing
        ):
            return False
        size = len(record.get("text", "")) + 16
        if chosen and used + size > character_limit:
            return False
        chosen.append(item)
        chosen_words.append(candidate_words)
        used += size
        return True

    for item in non_conversation:
        if try_add(item, non_conversation_budget):
            non_conversation_count += 1

    conversation_limit = non_conversation_count // 2
    for item in conversations:
        if conversation_count >= conversation_limit:
            break
        if try_add(item, CHAR_BUDGET):
            conversation_count += 1

    for item in non_conversation:
        if used >= CHAR_BUDGET:
            break
        if try_add(item, CHAR_BUDGET):
            non_conversation_count += 1

    if not chosen and conversations:
        try_add(conversations[0], CHAR_BUDGET)

    chosen.sort(key=lambda pair: pair[0], reverse=True)
    return chosen


def clusters(records: list[dict]) -> list[list[dict]]:
    """Greedy near-duplicate grouping by word-set Jaccard similarity."""
    groups: list[tuple[set[str], list[dict]]] = []
    for record in records:
        words = set(tokens(record.get("text", "")))
        if not words:
            continue
        for seed, members in groups:
            if len(words & seed) / len(words | seed) >= SIMILAR:
                members.append(record)
                break
        else:
            groups.append((words, [record]))
    return sorted((members for _, members in groups), key=len, reverse=True)


def clip(text: str, length: int = 150) -> str:
    text = " ".join(text.split())
    return text if len(text) <= length else text[: length - 1] + "…"


def report(agent: str, situation: str, now: dt.datetime) -> str:
    records = load(agent)
    lines = [f"## {agent}", ""]
    if not records:
        return "\n".join(lines + ["No memories.", ""])
    first, last = parse_time(records[0]["timestamp"]), parse_time(records[-1]["timestamp"])
    types = collections.Counter(r.get("type", "?") for r in records)
    tags = collections.Counter(t for r in records for t in r.get("tags", []))
    importance = collections.Counter(round(float(r.get("importance", 0.5)), 1) for r in records)
    reflections = [r for r in records if r.get("type") == "reflection"]
    groups = clusters(reflections)
    duplicated = sum(len(g) - 1 for g in groups if len(g) > 1)
    days = collections.Counter(parse_time(r["timestamp"]).date().isoformat() for r in records)

    lines += [
        f"- {len(records)} memories from {first:%Y-%m-%d} to {last:%Y-%m-%d}; busiest day {days.most_common(1)[0][0]} ({days.most_common(1)[0][1]}).",
        f"- By type: " + ", ".join(f"{name} {count}" for name, count in types.most_common()) + ".",
        f"- Importance: " + ", ".join(f"{value:.1f}: {count}" for value, count in sorted(importance.items())) + ".",
        f"- Top tags: " + ", ".join(f"`{name}` {count}" for name, count in tags.most_common(12)) + ".",
        f"- Reflections: {len(reflections)}, forming {len(groups)} distinct groups; "
        f"{duplicated} ({duplicated / max(1, len(reflections)):.0%}) repeat an earlier reflection closely (word overlap ≥ {SIMILAR:.0%}).",
        "",
        "### Most repeated reflections",
        "",
    ]
    for group in groups[:8]:
        if len(group) < 2:
            break
        lines.append(f"- ×{len(group)}, {group[0]['timestamp'][:10]} → {group[-1]['timestamp'][:10]}: “{clip(group[0]['text'])}”")

    chosen = retrieve(records, situation, now)
    chosen_ids = {id(r) for _, r in chosen}
    in_groups = sum(1 for g in groups if len(g) > 1 for r in g if id(r) in chosen_ids)
    chosen_types = collections.Counter(r.get("type") for _, r in chosen)
    oldest = min(parse_time(r["timestamp"]) for _, r in chosen)
    lines += [
        "",
        f"### What retrieval puts in the prompt ({len(chosen)} memories, as of {now:%Y-%m-%d %H:%M} UTC)",
        "",
        f"- By type: " + ", ".join(f"{name} {count}" for name, count in chosen_types.most_common()) +
        f"; {in_groups} belong to near-duplicate groups; oldest from {oldest:%Y-%m-%d}.",
        "",
    ]
    for value, record in chosen:
        lines.append(f"- {value:.2f} · {record['timestamp'][:10]} · imp {float(record.get('importance', 0.5)):.1f} · {record.get('type')}: {clip(record.get('text', ''), 120)}")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--agent", action="append", help="AgentId (repeatable); default: every Agents/*/memory.jsonl")
    parser.add_argument("--situation", default=DEFAULT_SITUATION, help="situation text to simulate retrieval against")
    parser.add_argument("--out", type=Path, help="report path (default Saved/MemoryReports/<date>.md)")
    parser.add_argument("--quiet", action="store_true", help="write the report without printing quoted memory text")
    args = parser.parse_args()

    agents = args.agent or sorted(p.parent.name for p in (ROOT / "Agents").glob("*/memory.jsonl"))
    now = dt.datetime.now(dt.timezone.utc)
    body = "\n".join(report(agent, args.situation, now) for agent in agents)
    text = f"# Resident memory review ({now:%Y-%m-%d})\n\nSimulated situation: {clip(args.situation, 400)}\n\n{body}"
    out = args.out or ROOT / "Saved" / "MemoryReports" / f"{now:%Y-%m-%d}.md"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8")
    if args.quiet:
        print(f"Written to {out}")
    else:
        print(text)
        print(f"\nWritten to {out}")


if __name__ == "__main__":
    main()
