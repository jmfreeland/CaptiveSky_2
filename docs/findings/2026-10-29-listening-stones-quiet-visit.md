# Listening Stones quiet-visit cue

## Behavior

An autonomous resident with an agent memory component can quietly linger within 1.9 m of the Listening Stones. Stillness is measured as horizontal and vertical velocity at or below 20 cm/s; the resident must remain in that close ring for 8 seconds. The Stones then produce one brief resonance and nearby autonomous residents who can hear it receive the existing private, deduplicated observation. Player pauses and ambient wildlife do not create a resident-etiquette memory.

The response uses the Stones' existing 45-second shared cooldown and remains transient. The resident is latched for the visit; walking beyond the existing 5.75 m rearm ring allows another visit to count. No actor is moved, no arrangement is created, and no model call is added.

## Validation

- The final UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked successfully in 4.77 seconds (5 UBT actions), including the presentation and its regression test; the preceding full rebuild compiled the chime changes as well.
- The automation regression now covers resident-only eligibility, stillness and range gates, the private memory description, one response during a continuous visit, and rearming after leaving the wider ring.
- The automation test itself was not run: the main editor has repeatedly exited before producing a fresh project log, and the repository crash finding says not to repeat identical editor or command-line startup attempts. Build success is compile evidence only, not live behavior or audible verification.

## Next check

When Unreal startup is available again, run `CaptiveSky2.Agent.ListeningStoneResidentMemory` in a bounded, request-disabled session. Confirm the response reads as a quiet acknowledgement rather than an alarm, the nearby resident remembers the witnessed behavior, and no second tone occurs while the visitor remains. Check the close-range stillness gate against actual resident movement before changing its threshold.
