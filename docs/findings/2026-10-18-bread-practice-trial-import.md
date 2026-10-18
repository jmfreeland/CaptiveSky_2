# Bread practice set: trial import (2026-10-18)

The revised offline bread prop (`SM_BreadPracticeSet_01`, see
[craft-lineage validation](2026-10-12-craft-lineage-validation.md)) is now imported
into Unreal as a trial. It is **not placed on the Island** and no map was opened or saved.

## What was done

- `Scripts/Import-ComfyBlenderAsset.py` gained `COMFYBLENDER_LIBRARY_DESTINATION`
  (default `Library`, simple folder name only). It redirects the shared `Library/Textures`
  import so a trial cannot replace textures other generated props already use.
- Import used a flat staging copy of the pack (the importer's asset-name check rejects
  nested paths) under `Saved/CompileScratch/BreadImportSource`, then ran headless with
  `COMFYBLENDER_DESTINATION=BreadTrial_20261018` and
  `COMFYBLENDER_LIBRARY_DESTINATION=Library_BreadTrial`.
- New local, gitignored content: `/Game/Generated/ComfyBlender/BreadTrial_20261018/`
  (mesh + 7 materials) and `/Game/Generated/ComfyBlender/Library_BreadTrial/`
  (Bread, Canvas, Wood maps). The existing `/Library` wood maps were not touched.

## Result

Exit code 0, `validation_errors: []`. Bounds 54 x 36 x 17.8 cm (manifest 54 x 36 x 18),
one embedded box collision, all seven material slots assigned. Log:
`Saved/Logs/Claude_BreadImport_20261018.log`. The commandlet ran beside the user's open
editor with about 2.9 GB free (user-approved); the editor stayed responsive.

## Not verified

- **Appearance under scene lighting.** A first thumbnail from the open editor was grey because the
  `-NullRHI` import left the materials uncompiled. After the editor compiled them, a second asset
  thumbnail shows the loaf, cut slice with crumb, folded linen and board with textures and the
  expected silhouette. That is a default thumbnail light, not the inn: board tone, crust colour and
  scale at the hearth still need a real-RHI look under the inn lighting before any placement.
- No Island placement, no canonical recipe, no visible-batch rule; those stay separate.
- The open editor may need an asset rescan to list the new folders.
