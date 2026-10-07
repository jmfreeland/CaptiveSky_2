# Wind Arch stone surface pass (2026-10-07)

The current runtime highlight showed the transient Wind Arch rock stacks as
very dark, high-contrast foreground forms. Their `SM_Rock` silhouette is useful,
but its default `M_Rock` surface reads as harsh black-white mottling and makes
the arch dominate the nearby shelter and residents.

`AWindArchStonework` now assigns a transient material instance based on the
Engine `BasicShapeMaterial`, with the same subdued warm-gray `Color` value
already used by the Listening Stones (`0.28, 0.26, 0.22`). The rough Starter
Content mesh, all 15 instance transforms, the saved cube proxies, collision,
navigation, and reversible Game/PIE presentation behavior are unchanged. If the
known engine surface cannot load, the transient presentation is skipped and the
saved proxies remain visible rather than silently restoring the stark default
material.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` Development build succeeded (8 UBT actions,
  including the Wind Arch presentation and automation-test translation units).
- `CaptiveSky2.Agent.WindArchPresentation` passed in headless automation. The
  updated test confirms a dedicated dynamic surface instance is used instead
  of the mesh's default material, alongside the existing 15-stone,
  collision/navigation, proxy-visibility, and natural-gust checks. Log:
  [`Codex_WindArchMaterial_20261007.log`](../../Saved/Logs/Codex_WindArchMaterial_20261007.log).
- Runtime appearance is **not yet verified**. The first bounded Game capture
  exited during startup because the installed DDC graph had no writable node.
  A retry with the memory-DDC fallback stalled in UE `TurnkeySupport` platform
  validation before loading the Island and was stopped after no log progress;
  it produced no screenshots. Logs:
  [`Codex_WindArchTintGame_20261007.log`](../../Saved/Logs/Codex_WindArchTintGame_20261007.log)
  and
  [`Codex_WindArchTintGame_MemoryDDC_20261007.log`](../../Saved/Logs/Codex_WindArchTintGame_MemoryDDC_20261007.log).

When the editor/Game launch path is available again, capture the same 17:00
Wind Arch route view and compare the shadowed stone brightness and resident
framing. This is a reversible material experiment, not a claim that the Arch's
oversized near-camera silhouette or the broader Island composition is solved.
