# Listening Stones surface override (2026-10-09)

The existing Game close-up at
[`001_Stone_Presentation_Close.png`](../../Playtests/Codex_StonePresentation_20261007/GameScreenshotsTintFinal/001_Stone_Presentation_Close.png)
still shows the transient cairns with strongly mottled black-and-white faces.
The `SM_Rock` asset preview confirms that this contrast belongs to its default
surface. The presentation created a warm-gray dynamic instance but assigned it
only to material slot 0, without checking for other mesh slots.

`AListeningStonePresentation::BuildStoneForms` now assigns that instance to
every static material slot on `SM_Rock`. If the known engine surface or dynamic
instance cannot be created, the transient replacement fails and the unchanged
map proxies remain visible. The focused regression logs the asset's slot count
and checks every slot on the transient component for a dynamic override.

## Verification and limits

- The two touched translation units compiled successfully with the cached UE
- A full UE 5.8.3 scratch editor build succeeded (47 actions), including
  `IslandListeningStonePresentation.cpp` and its test. `UBT AutoSDK ReturnCode`
  was 0.
- `CaptiveSky2.Agent.ListeningStonePresentation` passed. The regression logs
  one `SM_Rock` material slot and verifies it resolves to the transient dynamic
  stone surface. Log:
  [`Codex_ListeningStones_Override_Automation_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_ListeningStones_Override_Automation_20261009.log).
- The first editor-only viewpoint capture showed the unchanged blue map proxies,
  as expected: that preview does not start Game and therefore does not exercise
  the transient Game presentation. A bounded real-RHI Game run then loaded the
  current scratch binaries, replaced the proxies with the runtime cairns, and
  saved a warmed noon frame at
  [`002_Stone_Presentation_Close.png`](../../Saved/CompileScratch/Claude_Props/Saved/GameShots/Codex_ListeningStones_Override_Noon_20261009/002_Stone_Presentation_Close.png).
  It used an isolated data root, disabled thinking, a zero-request ceiling, and
  a 60-second real-time cap; it exited normally after 60.2 seconds with zero
  model requests. Log:
  [`Codex_ListeningStones_Override_GameNoon_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_ListeningStones_Override_GameNoon_20261009.log).
- The all-slot override is in effect, but the warmed frame still reads as
  strongly light/dark stacked rock. Since `SM_Rock` has only one material slot,
  extra slot coverage was not the visual bottleneck. The current frame does not
  distinguish whether the remaining contrast comes from the base surface,
  direct lighting, or self-shadowing at layer seams. Next: evaluate the
  candidate surface and the stack silhouette separately under matched daylight
  and shadow views before changing the tall landmarks again. Keep the imported
  small mossy `StoneCairn` prop separate; it is too wide to scale into these
  narrow 2.2–3.0 m proxies.

No map or Content asset was changed; the presentation remains transient and the
saved proxy actors remain authoritative. Scratch output uses
`Saved/CompileScratch/Claude_Props`; no user world state or open editor assets
were modified.
