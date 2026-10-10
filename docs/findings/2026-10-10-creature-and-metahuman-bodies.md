# Resident bodies: raven, rat and MetaHumans (2026-10-10)

Goal: bodies for the human residents (MetaHuman), the raven and the rat. Status: **raven and rat done and verified in UE 5.8.3; MetaHuman assets are created but their appearance is blocked on the MetaHuman optional content (a one-time Launcher install).**

## Raven: `/Game/Characters/Creatures/Raven/SK_Raven_01`
- Source: `Scripts/AssetPipeline/creatures/raven.py` reshapes the Fab Crow (vertex positions only, so skeleton, skin weights and UVs are the crow's): deep arched bill 14% longer, shaggy throat, wedge tail (centre feathers 30% longer, spread tightened), heavier neck/chest/head, thicker legs.
- Imported by `Scripts/Import-Creatures.py` **onto `SK_Crow_Skeleton`**, so every crow animation (idle, hop, takeoff, fly, landing) plays unchanged and the Raven controller's existing mapping works. Material: the existing `M_Crow_CaptiveSky` (renders near-black plumage).
- Verified: bounds match the crow's span (extent X 51.8 cm both; bill makes Y 27.4 vs 23.9); `ANIM_Crow_Walk` resolves to its skeleton; rendered in UE with `ANIM_Crow_IdleLookAround` (side and 3/4 views).
- Wired in (a7a7cac): `RavenAgentAIController.cpp` now tries `SK_Raven_01` first, then `SK_Crow_CaptiveSky`, then `SK_Crow`. Uncompiled and not run in-game; the controller scales the bird to a 112 cm span, so check it at gameplay scale.

## Rat: `/Game/Characters/Creatures/Rat/SK_Rat_01`
- Source: `creatures/rat.py` reshapes the Fab Fox (LOD0): thin bare tail, legs compressed toward the hip (about 46%) and slimmed, hunched plump body, round ears, narrow pointed snout. Slot 0 fur, slot 1 bare skin (tail, feet, nose). `creatures/rat_fur_texture.py` makes the fur BaseColor with numpy (no GPU).
- Imported **onto `SK_Fox_Skeleton`**: all Fox animations (idle, look-around, walk, run, sleep, rest) play on it. Materials built in `Import-Creatures.py`: `M_Rat_Fur` (texture) and `M_Rat_Skin`.
- Scale: use about 0.28x in the engine (the mesh is fox-sized, about 112 cm long including the tail, 38 cm tall). Place it by its bounds, like the raven code does (the legs are shortened, so the mesh does not stand at the fox's origin height).
- Resident body: `Scripts/Create-RatPlaceholder.py` creates `/Game/Agents/BP_Rat_Placeholder` (a copy of `BP_Agent_Placeholder` with `SK_Rat_01` at 0.28x, a small capsule, walk speed 140, yaw +90 so the fox-forward -Y faces +X). Spawned in a test world it stands upright and faces forward. Its animation is not driven yet (the placeholder has no locomotion AnimBP; the controller or an AnimBP must play the Fox sequences).
- Not placed: the user has now chosen a hidden fallen-wood hollow as the rat's first home. A clean identity/personality profile is active at `Agents/Agent_Rat_01/`, with no lived memory and no fabricated place coordinates. Nothing spawns a rat yet: first identify or author a distinct fallen-wood hollow anchor, then validate capsule/nav placement and wire the spawn. The read-only `Scripts/Inspect-IslandRatHollowCandidates.py` will shortlist map actors by explicit hollow tags and fallen-wood/log mesh-name hints; it does not modify the map, and its matches still require visual and navigation validation. Run it only as a bounded commandlet after closing the editor. Label its actor "Rat", never "Fenrus" (see `docs/residents/README.md`).
- Caveat: head is still fox-like (open-mouth pose from the Fox rest pose), the fur texture is procedural and does not follow the fox UV layout, so it is plain dark brown-grey. A hand-painted or ComfyUI pass is the next step.

## Human residents (Aster, Innkeeper): `/Game/Characters/MetaHumans/MHC_Aster`, `MHC_Innkeeper`
- Human-bodied residents with an `Agents/` identity are **Aster** and **the Innkeeper**. (`Character_6`/`Character_7`/test folders only hold `places.json`; "Rowan" is a prompt idea in `worldbuilding-ideas.md`, not an actor.) Pronouns are not specified for either resident, so the sex presentation in `Scripts/Create-MetaHumans.py` is a design choice, editable.
- `Scripts/Create-MetaHumans.py` creates the `MetaHumanCharacter` assets and applies body shape, skin tone and eye colour through the engine's MetaHumanGenerator helpers. Planned looks: Aster, young, lean, 172 cm, medium skin, hazel eyes; Innkeeper, sturdy, 166 cm, warm, weathered.
- **Blocker:** `unreal.MetaHumanGeneratorSubsystemWrapper.is_optional_content_installed()` is false here. Without it, the first skin/body commit hits `Assertion failed: BodyTexture` (MetaHumanCharacterBodyTextureUtils.cpp:109) and kills the editor, and the Character Editor cannot build. So the script currently saves **asset shells** only. Install the optional content (Epic Games Launcher, Library, UE 5.8, Options; engine `MetaHumanCharacter/Content/Optional` must exist), then re-run: `UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Create-MetaHumans.py -RenderOffscreen`. Assembling a playable MetaHuman (the Build step) may also need an Epic login and MetaHuman cloud services; that was not testable headless.
- Not wired in: the resident pawns still use their placeholder body.

## Reproduce
1. `Scripts/Export-CreatureReferences.py` exports `SK_Crow` / `SK_Fox` to FBX (needs `-RenderOffscreen`; NullRHI asserts on skeletal-mesh export).
2. `blender -b --factory-startup -P Scripts/AssetPipeline/creatures/raven.py -- SK_Crow.fbx <out>/Raven`, same for `rat.py`/`SK_Fox.fbx`, plus `rat_fur_texture.py`.
3. `UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Import-Creatures.py -RenderOffscreen` (set `CREATURE_SRC` when the project dir is not the repo).
