# Tideglass micro-scene study — 2026-11-23

A disposable Blender 5.2.2 Cycles study advances the November 1 proposal for one 4 m shoreline patch. The script uses the existing authored Tidepool Crab FBX plus existing Sea Thrift and Glasswort candidates, with a low 0.62 m camera, an irregular shallow water surface, submerged gravel, a few broken rock shelves, sparse algae and one restrained ripple cue. It writes only to the explicitly supplied output directory.

Preview: `Saved/WorldbuildingReviews/TidepoolMicroScene_20261123/TidepoolMicroScene.png` (local ignored review output). Reproduce with:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python Scripts/AssetPipeline/reviews/tidepool_micro_scene.py -- Saved/WorldbuildingReviews/TidepoolMicroScene_20261123
```

The render and its composition manifest were generated successfully. `python -m unittest Scripts.AssetPipeline.tests.test_tidepool_micro_scene` passes. The composition was visually inspected and iterated; it communicates shallow water and shore-scale fauna better than a wide landscape shot, but remains a stylized blockout. The rock forms and water shading need more authored surface character, and no crab detour, periwinkle grazing, minnow ripple, interaction, Unreal rendering, or performance result is represented here. No Island map or runtime asset was changed.
