# Listening Stones ecology response (2026-10-06)

The deterministic Island test now carries a natural rising-wind event through the transient Listening Stones chime and checks two local listeners. A firefly within the chime's audible radius begins its brief response-glow window and records that transient chime; a second firefly beyond the same radius stays unaffected. The existing assertions in this test also verify that a nearby resident receives a factual tone in its situation summary while a more distant resident does not.

Validation: the UE 5.8.3 headless `CaptiveSky2.Agent.ListeningStonePresentation` automation completed with `Result={Success}` and exit code 0 in [`Codex_StoneFireflyResponse_20261006.log`](../../Saved/Logs/Codex_StoneFireflyResponse_20261006.log). The run uses `-NullRHI -NoSound`, so this verifies deterministic proximity and response state, not the audible mix or how legible the firefly's glow response looks during interactive play. It also does not demonstrate that fireflies navigate toward the landmark; they answer nearby tones locally.

This adds no model call, persistent world state, or forced resident action. The remaining check is to view and listen in the UE 5.8.3 editor when the live editor window is available to this session.
