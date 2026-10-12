# PlantFactory content mount migration (2026-10-12)

## Result

The project-local `PlantFactoryPlugin` is now explicitly disabled in
`CaptiveSky_2.uproject`. Its directory and original assets remain in place as a
rollback source; this removes the deprecated content mount from normal project
startup without deleting licensed or user data.

Before disabling it, the two referenced MaterialFunctions,
`StaticBillboard` and `BillboardMappedNormals`, were copied to
`/Game/Generated/PlantMaterialCompatibility`. A disposable migration trial
compiled four duplicated materials with 8 remapped calls and no diagnostics.
The same remap was then applied to the three Island meadow master materials
(Festuca, Phalaris, Typha) and the existing generated Rhododendron repair-test
material. All 16 calls now resolve to project-local functions; every material
recompiled with an empty diagnostics list.

The Asset Registry audit after migration found 16 PlantFactory content assets,
zero `/Game` referencer packages, and zero audit errors. With the project
descriptor explicitly setting `PlantFactoryPlugin` to `Enabled: false`, a fresh
UE 5.8.3 `UnrealEditor-Cmd` verification reported zero mounted PlantFactory
assets, loaded all four migrated materials, recompiled them without diagnostics,
and loaded `/Game/Maps/Island` successfully. Model thinking and model requests
were disabled during the commandlet runs.

This was a `-NullRHI` load/compile verification. It does not replace a real-RHI
visual check of grass billboards in the editor/game. The next editor launch
should confirm the three meadow species still render as expected.

## Recovery

Byte-for-byte backups of the four changed material packages are in
`Saved/Backups/PlantFactoryMigration_20261012/`; the hashes were verified
against the originals before migration. To restore the previous descriptor
state, change the `PlantFactoryPlugin` entry in `CaptiveSky_2.uproject` to
`"Enabled": true`, then restore those four package files from the backup folder
if reverting the function remap as well.

The disposable trial assets remain under
`/Game/Generated/MaterialTests/PlantFactoryMigrationTrial_20261012`, with the
local function copies under `/Game/Generated/PlantMaterialCompatibility`.
Source scripts are `Scripts/Test-PlantFactoryContentMigration.py`,
`Scripts/Apply-PlantFactoryContentMigration.py`, and
`Scripts/Verify-PlantFactoryContentMigration.py`; the apply script requires the
explicit `CAPTIVESKY_APPLY_PLANTFACTORY_MIGRATION=1` environment gate.
