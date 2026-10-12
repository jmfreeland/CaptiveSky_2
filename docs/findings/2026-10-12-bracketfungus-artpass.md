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
a settled real-RHI in-game preview or Rat-capsule traversal test. Keep it as an
unplaced art candidate until those checks establish a convincing woodland
composition and safe collision.

**Isolated Unreal preview (2026-10-12):** duplicating the map and changing its
mesh in the same Editor-Cmd Python session hit UE 5.8's fatal `World Memory
Leaks` check. Staging the work across separate processes succeeded: a disposable
copy now lives at
`/Game/Generated/ComfyBlender/BracketFungusLog_ArtPass_20261012/Maps/BracketFungusLogArtPassPreview_20261012`.
It contains the new mesh and never alters `/Game/Maps/Island`.

A second isolated stage assigned the existing Common Hazel bark material to
the preview actor only. The assignment was confirmed in
`Saved/Logs/Codex_BracketFungusHazelBarkMaterialTest_20261012.log`. Its first
real-RHI Game run completed with a 60-second / zero-model-request cap and showed
`Required Pool = 4.96 MB` against the 4096 MB texture pool; this tiny prop scene
did not reproduce a texture-pool overage. However, the only screenshot still
had eight shaders preparing, so it is not a valid settled visual/material
comparison. The screenshot is
`Saved/Playtests/Codex_BracketFungusHazelBarkRealRHI_20261012/Screenshots/001_Bracket_Fungus_Log.png`.

The warmed repeat did not reach world-ready before its 300-second startup cap;
the script terminated only that Game process tree. Its log ends during
`Build.bat -Mode=ValidatePlatforms` / Turnkey startup, before map load or a
texture-pool sample:
`Saved/Logs/Codex_BracketFungusHazelBarkSettledRealRHI_20261012.log`. Thus the
Hazel override remains visually unverified, and the art candidate stays
unplaced. The Island remains untouched throughout.
