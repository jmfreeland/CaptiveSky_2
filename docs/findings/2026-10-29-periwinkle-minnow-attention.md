# Periwinkle attention to minnow surface breaks

Date: 2026-10-29

## Behavior

Shore periwinkles now briefly turn their eye stalks toward a nearby minnow that is actively making a surface break. The gaze uses the breaking fish's world location, rather than the minnow-school actor's center, and lingers for at most 0.9 seconds after the hop ends. Calm, submerged schools do not attract an arbitrary glance. Existing player attention still takes precedence, and the periwinkle does not move or persist any new memory as a result.

This is intentionally a small local cue: it helps the tidepool read as a connected habitat without making the snail chase fish or adding resident-model calls.

## Validation

- UE 5.8 Editor target compiled and linked in the isolated `Saved/CompileScratch/Claude_Props` build; the module and target timestamps are 2026-10-29 06:32.
- `CaptiveSky2.Agent.TidepoolPeriwinkleMinnowAttention` passed under NullRHI at 2026-10-29 06:53:08. The fixture verifies that calm fish are ignored, an active break attracts the gaze toward the fish, the linger expires, and the snail remains within its 16 cm home-patch excursion. The run used `-CaptiveSkyDisableAgentThinking -CaptiveSkyMaxModelRequests=0` and isolated `CaptiveSkyDataRoot` under ignored `Saved/CompileScratch`.

The automation exercises the interaction state in an in-world fixture, not its visual readability in PIE. The live editor was reopened afterward on the Island; no map or Content assets were changed for this feature.

## Follow-up

If the cue reads too mechanically in a real play session, tune the 220 cm attention radius or 0.9 second linger after observing it in-world. Keep player attention priority and the snail's bounded home area intact.
