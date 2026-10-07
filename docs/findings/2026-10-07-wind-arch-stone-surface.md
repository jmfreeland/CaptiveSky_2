# Wind Arch stone surface pass (2026-10-07)

The current runtime highlight showed the transient Wind Arch rock stacks as
very dark, high-contrast foreground forms. Their `SM_Rock` silhouette is useful,
but its default `M_Rock` surface reads as harsh black-white mottling and makes
the arch dominate the nearby shelter and residents.

`AWindArchStonework` now assigns a transient material instance based on the
Engine `BasicShapeMaterial`, with the same subdued warm-gray `Color` value
already used by the Listening Stones (`0.28, 0.26, 0.22`). The rough Starter
Content mesh, 15-instance count, stack positions/rotations, saved cube proxies,
collision, navigation, and reversible Game/PIE behavior are unchanged. A follow-
up profile multiplies only each pillar stone's horizontal X/Y scale by 0.72;
height, spacing, lintel geometry, and saved proxies are unchanged. If the known
engine surface cannot load, the transient presentation is skipped and the saved
proxies remain visible rather than silently restoring the stark default
material.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` Development build succeeded (8 UBT actions,
  including the Wind Arch presentation and automation-test translation units).
- After the cross-section adjustment, the incremental UE 5.8.3 build succeeded
  (5 UBT actions) and `CaptiveSky2.Agent.WindArchPresentation` passed again. Its
  test now asserts the 15 pillar instances keep the intended narrower profile,
  in addition to the existing material, proxy-height, collision/navigation,
  and natural-gust checks. Log:
  [`Codex_WindArchProfile_20261007.log`](../../Saved/Logs/Codex_WindArchProfile_20261007.log).
- `CaptiveSky2.Agent.WindArchPresentation` passed in headless automation. The
  updated test confirms a dedicated dynamic surface instance is used instead
  of the mesh's default material, alongside the existing 15-stone,
  collision/navigation, proxy-visibility, and natural-gust checks. Log:
  [`Codex_WindArchMaterial_20261007.log`](../../Saved/Logs/Codex_WindArchMaterial_20261007.log).
- A bounded, isolated Game run then verified the runtime presentation at 17:00:
  45.2 real seconds, no agent thinking or Python, and zero model requests. The
  [Wind Arch frame](../../Playtests/Codex_WindArchTint_Only_20261007/Screenshots/001_Wind_Arch_Overlook.png)
  shows the rough forms with a more even warm-gray surface rather than the
  previous stark mottling. Deep shadows remain, and the near-camera pillars
  still dominate the frame; this material pass did not change their scale. The
  screenshot also shows UE's existing competing-directional-lights warning, a
  separate lighting issue. Runtime log:
  [`Codex_WindArchTint_Only_Elevated_20261007.log`](../../Saved/Logs/Codex_WindArchTint_Only_Elevated_20261007.log).
- A same-camera, same-hour runtime comparison then used a 45-second cap with
  agent thinking and Python disabled; it ended after 45.3 seconds with zero
  model requests. The
  [narrower-profile frame](../../Playtests/Codex_WindArchProfile_20261007/Screenshots/001_Wind_Arch_Overlook.png)
  gives the nearby tree and shelter slightly more breathing room than the
  [material-only frame](../../Playtests/Codex_WindArchTint_Only_20261007/Screenshots/001_Wind_Arch_Overlook.png).
  The pillars still dominate much of the foreground, so 0.72 is a useful first
  pass, not a final scale decision. Log:
  [`Codex_WindArchProfileGame_20261007.log`](../../Saved/Logs/Codex_WindArchProfileGame_20261007.log).
- The first Game launch failed because the installed DDC graph had no writable
  node. A non-elevated retry with the memory-DDC fallback stalled in UE
  `TurnkeySupport` before loading the Island and was stopped after no log
  progress; it produced no screenshots. The elevated bounded run above passed
  that startup gate. Earlier logs:
  [`Codex_WindArchTintGame_20261007.log`](../../Saved/Logs/Codex_WindArchTintGame_20261007.log)
  and
  [`Codex_WindArchTintGame_MemoryDDC_20261007.log`](../../Saved/Logs/Codex_WindArchTintGame_MemoryDDC_20261007.log).

This is a reversible material and silhouette improvement, not a claim that the
Arch's near-camera scale, the forward-shading warning, or the broader Island
composition is solved.
