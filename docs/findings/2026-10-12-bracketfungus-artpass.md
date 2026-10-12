# Bracket-fungus log art pass — 2026-10-12

## Change

The first close review showed the shelf caps being lost against the branch.
The procedural source at `Scripts/AssetPipeline/assets/bracketfungus.py` now
uses a smoother 24-segment shelf silhouette, slightly broader caps with finer
growth ridges, and raises the three colonies toward the branch crown. The
bright moss tufts were reduced so the fungi read first. This is a source-level
revision; the original export and the earlier Unreal candidates remain intact.

## Validation

- Blender 5.2 built the revised FBX at 1.98 × 0.552 × 0.471 m and 2,130
  triangles. Its one box collider and UVMap were preserved.
- Blender's FBX re-import/render verifier reported `PROBLEMS []` and
  `VERIFY DONE`. The preview is
  `Saved/CaptiveSky/ComfyBlender/BracketFungusLog_ArtPass_20261012/BracketFungusLog/Preview.png`.
- UE 5.8.3 imported a separate candidate at
  `/Game/Generated/ComfyBlender/BracketFungusLog_ArtPass_20261012/`. The import
  report lists all five material slots, shared wood/moss textures, one box
  collider, expected dimensions and `validation_errors: []`.
- The 16 Python pipeline/world-interaction tests passed before this asset
  revision. Blender's own build and re-import verifier then exercised the new
  source directly.

Evidence: `Saved/CaptiveSky/ComfyBlender/BracketFungusLog_ArtPass_20261012/BracketFungusLog/unreal_import.json`
and `Saved/Logs/Codex_BracketFungusArtPassImport_20261012.log`.

## Limits and next step

No Island map or prior candidate was changed, and the new prop has not been
placed. The Blender preview shows the enlarged caps more clearly from above;
the low-angle preview still makes the bark grain look plank-like and the caps
remain modest. The Unreal import is structurally validated but has not yet had
a real-RHI in-game preview or Rat-capsule traversal test. Keep it as an
unplaced art candidate until those checks establish a convincing woodland
composition and safe collision.
