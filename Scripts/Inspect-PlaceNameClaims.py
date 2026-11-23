"""Read-only listing of attributed resident place-name claims in memory JSONL."""
import argparse
import json
import re
from pathlib import Path


def collect_claims(agents_dir: Path) -> list[dict[str, object]]:
    claims = []
    for memory_path in sorted(agents_dir.glob("*/memory.jsonl")):
        resident_id = memory_path.parent.name
        with memory_path.open("r", encoding="utf-8") as stream:
            for line_number, line in enumerate(stream, 1):
                if not line.strip():
                    continue
                try:
                    record = json.loads(line)
                except json.JSONDecodeError as error:
                    raise ValueError(f"{memory_path}:{line_number}: invalid JSON: {error}") from error
                if not isinstance(record, dict):
                    raise ValueError(f"{memory_path}:{line_number}: expected a JSON object")
                tags = record.get("tags", [])
                if not isinstance(tags, list) or "place-name" not in tags:
                    continue
                tag_values = [tag for tag in tags if isinstance(tag, str)]
                raw_text = record.get("text", "")
                text = raw_text if isinstance(raw_text, str) else ""
                label = next((tag.removeprefix("place-label:") for tag in tag_values
                              if tag.startswith("place-label:")), "")
                if not label:
                    quoted_label = re.search(r'call the place [^"]*"([^"]{1,40})"', text)
                    if quoted_label:
                        label = quoted_label.group(1)
                claims.append(
                    {
                        "resident": resident_id,
                        "timestamp": record.get("ts", record.get("timestamp", "")),
                        "place_id": next((tag.removeprefix("place-id:") for tag in tag_values
                                          if tag.startswith("place-id:")), ""),
                        "claimed_by": next((tag.removeprefix("claimed-by:") for tag in tag_values
                                            if tag.startswith("claimed-by:")), ""),
                        "learned_from": next((tag.removeprefix("learned-from:") for tag in tag_values
                                              if tag.startswith("learned-from:")), ""),
                        "label": label,
                        "conversation": next((tag.removeprefix("conversation:") for tag in tag_values
                                               if tag.startswith("conversation:")), ""),
                        "turn": next((tag.removeprefix("turn:") for tag in tag_values
                                      if tag.startswith("turn:")), ""),
                        "text": text,
                    }
                )
    return claims


def print_resident_knowledge(claims: list[dict[str, object]]) -> None:
    knowledge: dict[tuple[str, str], dict[str, set[str]]] = {}
    for claim in claims:
        place_id = claim["place_id"] if isinstance(claim["place_id"], str) else ""
        label = claim["label"] if isinstance(claim["label"], str) else ""
        resident = claim["resident"] if isinstance(claim["resident"], str) else ""
        if not resident:
            continue
        claimed_by = claim["claimed_by"] if isinstance(claim["claimed_by"], str) else ""
        learned_from = claim["learned_from"] if isinstance(claim["learned_from"], str) else ""
        if claimed_by:
            role = f"claimed by {claimed_by}"
        elif learned_from:
            role = f"learned from {learned_from}"
        else:
            role = "recorded"
        knowledge.setdefault((place_id, label), {}).setdefault(resident, set()).add(role)

    if not knowledge:
        print("No resident place-name knowledge found.")
        return
    print("Resident-held labels (recorded knowledge only; agreement or adoption is not inferred):")
    for (place_id, label), residents in sorted(knowledge.items(), key=lambda item: (item[0][0].casefold(), item[0][1].casefold())):
        name = f'“{label}”' if label else "(label unavailable)"
        holders = ", ".join(
            f"{resident} ({'; '.join(sorted(roles))})"
            for resident, roles in sorted(residents.items(), key=lambda item: item[0].casefold())
        )
        print(f"{place_id or 'unknown place'}: {name} — {holders}.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--agents-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "Agents",
        help="Agents folder (defaults to the repository Agents directory)",
    )
    output = parser.add_mutually_exclusive_group()
    output.add_argument("--json", action="store_true", help="write the claims as JSON")
    output.add_argument("--knowledge", action="store_true",
                        help="group distinct residents by place and personal label without inferring agreement")
    args = parser.parse_args()
    claims = collect_claims(args.agents_dir)
    if args.json:
        print(json.dumps(claims, ensure_ascii=False, indent=2))
        return
    if args.knowledge:
        print_resident_knowledge(claims)
        return
    if not claims:
        print("No resident place-name claims found.")
        return
    for claim in claims:
        source = claim["claimed_by"] or f"learned from {claim['learned_from']}"
        place_name = f'“{claim["label"]}” for {claim["place_id"]}' if claim["label"] else claim["place_id"]
        print(f"{claim['resident']} knows {place_name} ({source}; "
              f"conversation {claim['conversation']} turn {claim['turn']}): {claim['text']}")


if __name__ == "__main__":
    main()
