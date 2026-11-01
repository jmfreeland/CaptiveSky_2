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
multiple material functions under
`/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/` (for
example `MF_adjustAO`, `MF_adjustBaseColor`, and `MF_adjustNormal`). Those
packages are skipped because the Procedural Vegetation Editor plugin is not
mounted in this game run. The same log cannot load native PVE graph assets
because the `/Script/ProceduralVegetation` classes are unavailable. These
warnings do not prove what the rendered bark looks like, but they do prove the
duplicated bark parent is not dependency-self-contained for a plugin-free game
runtime.

Next asset milestone: replace the runtime bark parent with a project-owned,
self-contained material (or project-own every required dependency), then
validate its material compile and a small controlled tree preview. Keep PVE
graph assets editor-only; do not depend on the PVE plugin's sample content in
the runtime material. The open Unreal editor was left untouched, and no
Content assets were changed during this playtest.
