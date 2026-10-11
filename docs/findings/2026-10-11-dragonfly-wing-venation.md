# Tideglass dragonfly wing venation — 2026-10-11

## Change

The daytime dragonfly's four translucent wings used to render as plain leaf-like
surfaces. Each procedural wing now has a second, lightweight mesh section with
three longitudinal veins and nine cross-veins on both sides. Vein vertices
follow the existing arched membrane, use a darker translucent tint, and remain
part of the same collisionless, shadowless component. No body, flight, habitat,
interaction, save-state, or model-request behavior changed.

The NightEcology automation now checks that the vein section has finite,
collision-free geometry and a darker translucent tint. The mesh remains a
stylized procedural placeholder; this pass improves the silhouette/detail but
does not replace a purpose-authored insect model or material.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor Win64 Development` build succeeded.
- `CaptiveSky2.Agent.NightEcology` passed in NullRHI with agent thinking
  disabled and the model-request cap set to zero:
  `Saved/Logs/Codex_DragonflyVenation_NightEcology_20261011.log`.
- The editor-only `CaptiveSky2.Visual.Viewpoints` capture passed with
  `-ViewpointHour=11 -ViewpointOnly=Tideglass -ViewpointGroundCover
  -ViewpointTideglassDragonflies -ViewpointNoWorldState`. It spawned three
  transient dragonflies and confirmed the wet edge's seven Typha plus three
  Phalaris instances; preview actors were destroyed after capture.
- Dense ground-cover scene-capture p95 was 42.57 FPS at the Tideglass overview,
  39.19 FPS at ground detail, and 41.14 FPS in the dragonfly close view. All
  exceed the 30-FPS automation screen, but are not Game/PIE, packaged-build, or
  target-hardware guarantees.

Log: `Saved/Logs/Codex_DragonflyVenation_Viewpoints_20261011.log`.
Images: `Saved/Viewpoints/2026-10-11_182223_h11.0/` (`02_Tideglass.png`,
`02a_TideglassGroundDetail.png`, and `02e_TideglassDragonflyClose.png`).

## Remaining art work

The close frame confirms the wing pattern is visible. The body and membrane
shape are still deliberately simple, while the wide Tideglass shot remains
dominated by proxy-white pool/landmark surfaces and coarse exposed soil. A
purpose-authored dragonfly asset/material and the active landscape/landmark art
pass remain separate follow-ups.
