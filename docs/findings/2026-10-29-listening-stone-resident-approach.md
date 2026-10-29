# Listening Stones: quiet resident approaches

Conscious residents can now elicit one brief Listening Stones response when they
move quietly and deliberately toward the landmark. This gives residents a small,
observable way to engage with a place without adding a model call or a scripted
conversation.

The approach gate is limited to non-player pawns carrying `UAgentMemoryComponent`
within the existing notice radius, height band, and maximum approach speed. A
resident must move at least 35 cm/s, with at least 0.55 directional alignment
toward the Stones. Resting, crossing past, or walking away does not qualify.
Ordinary wildlife remains excluded. Player-controlled visitors retain their
existing presence-based response. Existing latch, active-resonance, and shared
chime cooldowns still prevent repeated tones.

Resident approaches have a distinct chime cause. Nearby conscious residents who
can hear it record one short private observation; similar observations remain
deduplicated over the existing twelve-hour window. The response is transient and
does not promise a discovery or create a puzzle.

## Verification

- UE 5.8 isolated scratch editor target built and linked successfully.
- `CaptiveSky2.Agent.ListeningStoneResidentMemory` passed under NullRHI, including
  the actual directed-resident approach, distinct response, private memory, and
  latch/no-repeat path.
- Thinking was disabled, the model-request cap was zero, and test data was routed
  to the ignored scratch directory.
- No map, Content asset, live editor state, or PIE session was changed for this
  milestone. A visual or in-world play check remains outstanding.
