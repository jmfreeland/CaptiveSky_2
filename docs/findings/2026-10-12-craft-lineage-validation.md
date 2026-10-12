# Aster's craft lineage: current implementation and validation

## Current state

The Island world-state subsystem contains a persistent, renderer-independent
craft ledger within Unreal. A tradition keeps an ID, display name, originating
resident, source/inspiration, bounded lesson lineage, and bounded practice
history. An attempt records who practiced, Island day, up to four normalized
settings, and an explicit outcome (`Unrated`, `Promising`, or `Successful`).
The ledger is bounded to 16 traditions, 32 lessons per tradition, and 24
attempts per tradition; the normalized settings are limited to `hydration`,
`rise`, `heat`, and `salt`.

The first practice is not pre-seeded. A resident can choose `practice_craft` at
the visible inn hearth only while it is lit and the resident is nearby. The
first record is explicitly `Unrated`; a resident can refine a tradition only
after acquiring knowledge of it. The current subsystem records no ingredients,
loaf actor, inventory transfer, or automatic quality judgement.

Teaching is optional, resident-to-resident, and attributed. The teacher must
know the named tradition; the learner gains it only when the existing
conversation path delivers speech in range. It does not fire for player or
external-channel conversation. Craft practice/teaching use the normal scheduled
resident decision path and do not launch a second model request.

This is a renderer-independent record inside Unreal's world-state subsystem,
not yet the engine-agnostic canonical service proposed in the worldbuilding
plan. It preserves the intended distinction between a memory of practice and
a physical object: no recipe is given to Aster at startup, and an unrated
experiment is not represented as a successful loaf.

## Validation

The UE 5.8.3 Editor target build completed successfully and was up to date. The
isolated `Claude_Props` scratch project loads the shared project source and
separate binaries; the focused NullRHI automation run used a 60-second ceiling,
disabled resident thinking, allowed zero model requests, and wrote world data
under a dedicated playtest directory. All three focused tests passed:

- `CaptiveSky2.Agent.IslandCraftTradition` — bounded attempts, attributed
  lessons, and persistence across state-file reload.
- `CaptiveSky2.Agent.CraftTeaching` — only a known tradition can be shared, only
  through the delivered nearby resident conversation, with no extra model call.
- `CaptiveSky2.Agent.IslandInnHearth` — optional first practice requires the
  lit hearth and records an unrated attempt without creating bread or inventory.

Log: `Saved/Logs/Codex_CraftLineageValidation_20261012.log`.

The log also contains `UWorld::DestroyActor: World has no context!` warnings
while the transient hearth test world is cleaned up; the automation controller
reported `Success` for that test. Revisit cleanup warnings if they begin
affecting tests or runtime worlds.

## Next visible milestone

The revised bread practice prop remains a separate offline FBX preview at
`Saved/CaptiveSky/ComfyBlender/BreadPracticeTrial_20261012/BreadPracticeSet/`;
the revised export is not yet imported into Unreal or placed on the Island.
When the editor is stable, import it to a new trial destination and review the
board, crumb, linen, and scale under the inn's actual lighting. Keep that visual
review separate from approving a canonical recipe or saving an Island
placement. A later milestone can let Aster voluntarily create a visible batch
from an unrated attempt, but only after the world has explicit rules for the
resulting object and its persistence.
