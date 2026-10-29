# Broadleaf canopy profiles (2026-10-29)

## Change

Runtime broadleaf placement previously scaled maple, aspen, beech, and black
alder to the same 9.5–17 m height range. That gave the four-species generator
less visible vertical structure than its species counts suggest. Each
species now has a bounded, seeded height profile while retaining overlap:

| Species | Height range |
| --- | ---: |
| Norway maple | 10–15 m |
| European aspen | 12.5–18 m |
| European beech | 10.5–16 m |
| Black alder | 9–14 m |

The ranges add a taller aspen layer and lower moisture-edge alder layer without
changing tree placement, per-species quotas, the 3,600 broadleaf-instance
budget, collision, or cull settings. No new assets or runtime instances were
added. The seeded random stream supplies the within-species height sample.

## Validation and limits

- UE 5.8 `CaptiveSky_2Editor` compiled and linked successfully in this
  worktree after adding pure automation coverage for profile endpoints,
  midpoint, clamping, and invalid inputs.
- A follow-up headless NullRHI automation launch crashed before engine init at
  `SecondsSinceStart=0`, with the same access-violation call-stack hash
  `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D` recorded in the prior isolated
  startup attempt. The new automation tests therefore compiled but did not
  execute. Do not repeat this identical isolated launch without investigating
  the startup path.
- The main project editor is running a different checkout; it has not loaded
  this source change. No PIE, asset save, or resident-model request was used
  for this milestone. Species silhouettes and visual naturalness remain to be
  checked in the main project at a daylight viewpoint when GPU headroom allows.

## Next

Integrate the source change into the project editor's checkout, then make a
short daylight capture with resident thinking disabled. Compare maple/aspen/
beech/alder silhouettes and record FPS and whole-device VRAM separately from
the existing texture-pool measurements. Preserve the fixed tree budget and
30-FPS target while judging the result.
