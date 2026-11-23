"""Read-only listing of attributed resident place-name claims in memory JSONL."""
import argparse
import json
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
                tags = record.get("tags", [])
                if not isinstance(tags, list) or "place-name" not in tags:
                    continue
                tag_values = [tag for tag in tags if isinstance(tag, str)]
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
                        "conversation": next((tag.removeprefix("conversation:") for tag in tag_values
                                               if tag.startswith("conversation:")), ""),
                        "turn": next((tag.removeprefix("turn:") for tag in tag_values
                                      if tag.startswith("turn:")), ""),
                        "text": str(record.get("text", "")),
                    }
                )
    return claims


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--agents-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "Agents",
        help="agent data directory (defaults to the repository Agents directory)",
    )
    parser.add_argument("--json", action="store_true", help="write the claims as JSON")
    args = parser.parse_args()
    claims = collect_claims(args.agents_dir)
    if args.json:
        print(json.dumps(claims, ensure_ascii=False, indent=2))
        return
    if not claims:
        print("No resident place-name claims found.")
        return
    for claim in claims:
        source = claim["claimed_by"] or f"learned from {claim['learned_from']}"
        print(f"{claim['resident']} knows {claim['place_id']} ({source}; "
              f"conversation {claim['conversation']} turn {claim['turn']}): {claim['text']}")


if __name__ == "__main__":
    main()
