# Resident feedback and rain-basin playtest (2026-11-01)

## Run and safety result

`Codex_ResidentFeedback_OceanRebase_20261101.log` records a 900.2-second
NullRHI resident run. The real-time guard ended it normally at the configured
15-minute cap with 19 model requests and 0 in flight; no automatic restart was
requested. This validates bounded behavior and simulation logic only, not
render quality, frame rate, or GPU memory.

The same initialization logged 61,417 ground-cover instances, 15,513 trees
(8,537 saplings), 15,555 broadleaf shrubs, and 1,489 flowering rhododendrons.
The mature mix included 900 each of Norway maple, European aspen, European
beech, and black alder across 372 of 500 groves, plus 36 birch coppices and 60
hazel coppices. A/B variants were close to evenly represented for all four
Megaplant species. This confirms ecological spawn diversity in the runtime;
NullRHI cannot tell us whether those canopies look healthy or naturally
scattered on screen.

## What residents did

- Aster opened a conversation with Raven about the rain and the pool. They
  exchanged four turns over roughly three minutes, then the conversation closed
  with a cooldown. The exchange was reciprocal and did not loop.
- The rain basin was correctly described as dry during the initial dry-weather
  visit; Raven's attempted leaf interaction changed nothing. After a shower
  filled it, Rat and Raven each chose to set a leaf afloat. The basin reported
  two drifting leaves. This is a useful, persistent-feeling interaction whose
  availability followed the actual weather state.
- All four residents received the one-time optional feedback invitation. The
  logs say each made no spoken response to that prompt; they continued making
  ordinary choices, so the invitation did not force a request or interrupt
  play. No executed `request_upgrade` or `request_object` appears in this run.
- A direct Rat-to-basin approach was blocked by nearby StarterContent rocks.
  The recovery search first found no capsule-clear candidate, then selected a
  nearby staging point and later found a clear route. Raven's collision body
  also blocked a sweep briefly. Recovery worked, but landmark paths still have
  avoidable scenery conflicts and extra movement.

## Asset-loading issue surfaced by the runtime

The `-game` log reports that project-owned
`/Game/Megaplant_Library/Materials/MA_Foliage_Trees_InstancedSkinned` depends on
many packages under the plugin's sample-material tree, including material
functions (`MF_adjustAO`, `MF_adjustBaseColor`, `MF_adjustNormal`, and others),
virtual textures, and a material-parameter collection. They are skipped
because the Procedural Vegetation Editor plugin is not mounted in this game
run. The log also shows the Norway maple, aspen, and beech foliage instances,
plus both Black Alder bark and foliage instances, still reference the plugin's
`MA_Foliage_Trees` parent. The existing repair script only reparents the bark
instances for the first three species, and its copied parent is not
self-contained. The same log cannot load native PVE graph assets because the
`/Script/ProceduralVegetation` classes are unavailable. These warnings do not
prove what the rendered trees look like, but they establish that runtime tree
materials still reach into editor-plugin content.

`Scripts/Inspect-MegaplantRuntimeMaterialDependencies.py` was run read-only in
the open UE 5.8 editor with PVE mounted. The report is in
`D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/Logs/CaptiveSky_2_2.log` at
13:56:00 (`MegaplantRuntimeMaterialAudit`). All eight A/B meshes loaded and all
material slots were inspected without errors. The recursive closure contains
24 unique `/ProceduralVegetationEditor/SampleAssets/...` packages: the PVE
master material, three virtual textures, nineteen material functions, and
`MPC_GlobalFoliageActor`.

Every A/B variant has the same parent pattern. Norway Maple, European Aspen,
and European Beech bark instances use the project-owned
`/Game/Megaplant_Library/Materials/MA_Foliage_Trees_InstancedSkinned`, but
their foliage instances still parent directly to PVE's
`MA_Foliage_Trees`. Both Black Alder slots (bark and foliage) still parent
directly to that PVE material. Crucially, the project-owned bark parent itself
still has the complete PVE dependency closure; duplicating only the master and
reparenting instances is not a runtime fix. There are 10 direct PVE-parented
slots across the eight meshes, plus six bark slots whose apparently
project-owned parent still resolves into PVE content.

Next asset milestone: make a self-contained runtime foliage material and
remap the species instances to it, or copy and remap the *entire* dependency
closure into project content. Validate compiled materials and a small
controlled tree preview before rolling the change into Island distribution.
Keep PVE graph assets editor-only; runtime materials must not rely on the
editor plugin's sample content. This audit did not save or modify Content
assets; the editor's current Island scene remains open.
