# Resident memory review (2026-09-27)

Produced with `Scripts/Analyze-AgentMemory.py`. The full report quotes private memories and is written under `Saved/MemoryReports/`; this summary uses aggregates only.

## What the memories contain

| | Aster | Raven |
|---|---|---|
| Memories | 918 | 817 |
| Reflections (model-written) | 824 | 732 |
| Conversation lines | 91 | 83 |
| Observations (action results) | 3 | 2 |
| Reflections closely repeating an earlier one (word overlap ≥ 60%) | 715 (87%) | 546 (75%) |
| Largest single repeated reflection | ×187 | ×54 |

Nearly all of the repetition comes from the long 13–14 September runs. The residents rewrote the same "I moved to inspect the roost's shelter before choosing where to settle" reflection at every think, lightly reworded.

## Why the latest talk keeps repeating

`UAgentMemoryComponent::GetRelevantContext` scores each memory as 0.4 × recency (24-hour half-life), plus 0.4 × importance, plus 0.2 × keyword overlap. It then fills about 3200 characters. Every spoken line is stored as its own memory with importance 0.5, so during and after an exchange the freshest lines win every slot. Simulated against a typical afternoon situation:

- Aster's 19 retrieved memories are all today's sky and pool small talk.
- 16 of the raven's 18 are the same exchange.

The model sees nothing but the current conversation and continues it. This is one of the reasons the live sessions drifted into long agreeable exchanges (see README, "Grounding audit and the first live session"). The other reason, pacing between exchanges, was addressed separately in `148f0ea`.

## Recommended changes (need a build and a live check)

1. **Spread the prompt across kinds of memory.** In `GetRelevantContext`, cap conversation lines at about a third of the chosen records, and skip any candidate too similar to one already chosen (word-set Jaccard ≥ 0.6). The gateway now does exactly this for headless replies: `Gateway/src/CaptiveSky.Gateway/Agents/MemoryContextSelector.cs`, commit `4d0f3ee`.
2. **Store small talk as less important.** Keep agent-to-agent lines at importance 0.3 rather than 0.5, so distinctive experiences can outrank them. Longer term, one consolidated summary per exchange instead of every line.
3. **Refuse near-duplicate reflections at write time.** In `ParseDecisionAndStoreMemories`, skip a new reflection that closely matches one from the last few hours. That stops the 187-copy pattern at the source.
4. **Measure before and after.** Rerun the script after a live session and compare the duplicate rates and the mix of retrieved memories.

None of this deletes or rewrites existing memories. The history stays as it is, and retrieval simply chooses better from it.
