# A Tideglass frog watches a nearby minnow surface break

When a school minnow actually breaches the surface nearby, the Common Frog can
briefly turn toward the breaking fish. The signal is limited to 1,100 cm
horizontally and 500 cm vertically, lasts 1.8 seconds, and is sampled alongside
the frog's existing 0.30-second wildlife check. It also requires a clear
visibility trace to the fish. A 12-second shared attention
cooldown and per-school latch prevent the same surface-break episode from
restarting the cue. The frog does not hop, create another ripple, change its
home, or call an LLM. Dragonfly and minnow cues share the same attention state,
so the frog tracks one nearby event at a time.

The isolated UE 5.8 editor target built and linked successfully (21 actions).
After adding the visibility trace, the incremental rebuild also succeeded
(4 actions).
With `-NullRHI`, agent thinking disabled, zero model requests, and scratch-only
world data, `CaptiveSky2.Agent.Ecology.CommonFrogMinnowSurfaceBreak` passed
against the final source with `Result={Success}` and exit code 0. Its transient
box fixture first confirmed that an opaque obstacle intercepts the visibility
trace, then verified that the frog ignores the breach until the obstacle is
removed. The existing
`CaptiveSky2.Agent.Ecology.CommonFrogDragonflyAttention` regression also passed
with the shared-attention refactor. Logs:

- `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_FrogMinnow_20261029_0638.log`
- `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_FrogDragonfly_20261029_0620.log`

These tests validate the bounded cue and its rearm/cooldown behavior, not its
readability in a rendered Island view or the frequency of minnow breaches in
ordinary play. Check the shore scene in a short zero-request session before
tuning the attention duration or cadence.
