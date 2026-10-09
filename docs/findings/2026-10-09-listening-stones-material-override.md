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
  5.8.3 response files and the installed MSVC 14.44 toolchain. This is syntax
  and translation-unit compile evidence only; no module link or automation test
  ran.
- A normal scratch `CaptiveSky_2Editor` build started at 03:01 local but stalled
  in UBT's bundled .NET platform-validation child before compiler output. Its
  `AutoSDKInfo.txt` remained unchanged from the previous day. Only the two
  processes from that scratch build were stopped; the open editor was not
  touched. This reproduces the earlier Turnkey startup problem, not a source
  compile failure.
- No new Game/PIE capture was taken, so the surface's in-world appearance is
  still unverified. Once UBT validation proceeds, build the scratch target, run
  `CaptiveSky2.Agent.ListeningStonePresentation`, and take a matched Listening
  Stones close-up to confirm the default texture no longer leaks through.

No map or Content asset was changed; the presentation remains transient and the
saved proxy actors remain authoritative.
