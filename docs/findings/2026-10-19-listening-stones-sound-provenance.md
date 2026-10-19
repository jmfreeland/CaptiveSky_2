# Listening Stones chime provenance

## Change

Transient Listening Stones tones now retain why they happened: a rising natural
gust, a quiet visitor approaching, or a close inspection. The existing nearby
resident observation distinguishes these causes while the tone is audible and
for its existing short-lived context window. Unknown/staged tones keep the
generic description.

This is descriptive context only. It adds no model request, persistent memory,
world-state write, extra actor tick, or change to the sound's range, duration,
or cooldown. The gust, visitor, and inspection paths remain transient.

## Verification

- UE 5.8.3 Editor target built successfully from the isolated
  `Codex_CurrentSourceBuild_20261018` scratch project (18 actions, 93.24 s).
- `IslandListeningStonePresentationTests.cpp` now checks that resident-facing
  descriptions distinguish all three causes, alongside the existing transient
  lifetime, distance, cooldown, and wildlife-response checks.
- The Unreal automation test was not run: the shared coordination note still
  forbids launching another project process while the unsymbolized pre-init
  crash is unresolved. The editor is open and responsive, but its MCP transport
  at `127.0.0.1:8000` is currently unavailable.

## Next check

When the editor automation connection is available again, run
`CaptiveSky2.Agent.ListeningStonePresentation` without enabling resident
thinking or model requests. Then verify in a short, bounded play session that
residents naturally receive the right explanation for a visitor-triggered tone
and for a wind-triggered tone.
