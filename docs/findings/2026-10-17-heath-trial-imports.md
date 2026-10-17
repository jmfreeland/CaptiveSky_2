# Heath species trial imports — 2026-10-17

Two local botanical candidates now have Unreal package trials in the main
project's ignored `Content/Generated/ComfyBlender/` tree:

- `CoastalLingHeather_Trial_20261017` — 44.6 × 54.4 × 39.3 cm, about 5,112
  triangles.
- `CoastalCrossLeavedHeath_Trial_20261017` — 46.2 × 52.8 × 38.9 cm, about
  5,026 triangles.

Both were imported with the existing `Scripts/Import-ComfyBlenderAsset.py` into
new folders in the disposable UE 5.8.3 `FabUsdProbe` project, using NullRHI,
then copied as four `.uasset` packages per candidate (one mesh and three
materials) to the corresponding main-project trial folders. Neither import
uses external textures. The import reports record all three material slots,
zero collision shapes, dimensions within manifest tolerance, and
`validation_errors: []`. UE's FBX importer did warn that the files contain no
smoothing-group information; their FBX normals were exported, but shading has
not yet been judged in Unreal.

This proves package generation and import-structure validation, not that the
main project loads the copied packages. The main project's project-open crash
remains unresolved; the Island map was not loaded, changed, or saved. The
existing Blender previews show the candidates are still stylized, relatively
open branching clumps. They are unplaced trials, not approved final flora.
When the main editor is healthy, inspect their materials and normals in a
representative wet-heath/woodland composition, then test a small grouped patch
at gameplay distance before any Island placement. Do not remove or replace
Fab Megaplant/USD content based on these trials.

Local evidence (ignored by Git):

- `Saved/CaptiveSky/ComfyBlender/CoastalLingHeather/unreal_import.json`
- `Saved/CaptiveSky/ComfyBlender/CoastalCrossLeavedHeath/unreal_import.json`
- `Saved/CompileScratch/FabUsdProbe_20261010/Saved/Logs/Codex_CoastalLingHeather_ImportProbe_20261017.log`
- `Saved/CompileScratch/FabUsdProbe_20261010/Saved/Logs/Codex_CoastalCrossLeavedHeath_ImportProbe_20261017.log`
