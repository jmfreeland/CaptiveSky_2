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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--agents-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "Agents",
        help="Agents folder (defaults to the repository Agents directory)",
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
        place_name = f'“{claim["label"]}” for {claim["place_id"]}' if claim["label"] else claim["place_id"]
        print(f"{claim['resident']} knows {place_name} ({source}; "
              f"conversation {claim['conversation']} turn {claim['turn']}): {claim['text']}")


if __name__ == "__main__":
    main()
