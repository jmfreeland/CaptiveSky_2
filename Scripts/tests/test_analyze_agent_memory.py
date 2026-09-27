"""Synthetic coverage for the memory analyzer's retrieval simulation."""
from __future__ import annotations

import datetime as dt
import runpy
from pathlib import Path
import sys
import unittest


sys.dont_write_bytecode = True
ANALYZER = Path(__file__).resolve().parents[1] / "Analyze-AgentMemory.py"
MEMORY = runpy.run_path(str(ANALYZER), run_name="memory_analyzer")


def record(kind: str, text: str, now: dt.datetime) -> dict:
    return {
        "type": kind,
        "text": text,
        "timestamp": now.isoformat(),
        "importance": 0.5,
        "tags": [],
    }


class RetrieveTests(unittest.TestCase):
    def setUp(self) -> None:
        self.now = dt.datetime(2026, 9, 27, 12, tzinfo=dt.timezone.utc)

    def test_distinct_experience_precedes_dialogue_and_dialogue_is_bounded(self) -> None:
        records = [
            record("observation", f"observationtoken{index} weathercondition{index}", self.now)
            for index in range(6)
        ]
        records.extend(
            record("conversation", f"dialoguetoken{index} greeting{index}", self.now)
            for index in range(12)
        )

        selected = MEMORY["retrieve"](records, "island weather", self.now)
        non_conversations = sum(item[1]["type"] != "conversation" for item in selected)
        conversations = sum(item[1]["type"] == "conversation" for item in selected)

        self.assertGreaterEqual(non_conversations, 6)
        self.assertLessEqual(conversations, non_conversations // 2)

    def test_near_duplicate_records_are_not_both_selected(self) -> None:
        records = [
            record("observation", "pale stones reveal a quiet trail", self.now),
            record("reflection", "pale stones reveal quiet trail", self.now),
            record("observation", "the wind moves through the high grass", self.now),
        ]

        selected = MEMORY["retrieve"](records, "pale stones trail", self.now)
        selected_texts = [item[1]["text"] for item in selected]

        self.assertEqual(len(selected_texts), 2)
        self.assertEqual(
            sum(text in selected_texts for text in ("pale stones reveal a quiet trail", "pale stones reveal quiet trail")),
            1,
        )

    def test_conversation_only_history_has_a_fallback(self) -> None:
        conversation = record("conversation", "a lone remembered greeting", self.now)

        selected = MEMORY["retrieve"]([conversation], "the Island", self.now)

        self.assertEqual([item[1] for item in selected], [conversation])


if __name__ == "__main__":
    unittest.main()
