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
- Resident body: `Scripts/Create-RatPlaceholder.py` creates `/Game/Agents/BP_Rat_Placeholder` (a copy of `BP_Agent_Placeholder` with `SK_Rat_01` at 0.28x, a small capsule, walk speed 140, yaw +90 so the fox-forward -Y faces +X). `ARatAgentAIController` selects the Fox breathing idle, occasional look-around, walk, run, and sleep sequences from movement/rest state; the body-creation script assigns that controller class. The UE 5.8 editor target compiled in an isolated scratch project with its own binaries. `CaptiveSky2.Agent.RatAnimationAssets` loaded the actual Rat mesh and all five Fox sequences, confirmed their skeleton compatibility, possessed a transient Rat body, and exercised idle, walk, run, sleep, and wake transitions while asserting the active sequence each time. `CaptiveSky2.Agent.RatLocomotionAnimation` covers speed/rest state selection. Both passed in `Saved/CompileScratch/RatAnimationProbe_20261010/RatAnimationAutomation.log` with NullRHI, thinking disabled, zero model requests, a 60-second cap, and scratch-only WorldState. Still unverified: rendered walk/run/sleep transitions, and re-running the body-creation script to assign the controller to the existing local Blueprint asset. The live Island editor remained open; no map or Content asset was changed.
- Not placed: the user has now chosen a hidden fallen-wood hollow as the rat's first home. A clean identity/personality profile is active at `Agents/Agent_Rat_01/`, with no lived memory and no fabricated place coordinates. Nothing spawns a rat yet: first identify or author a distinct fallen-wood hollow anchor, then validate capsule/nav placement and wire the spawn. The read-only `Scripts/Inspect-IslandRatHollowCandidates.py` will shortlist map actors by explicit hollow tags and fallen-wood/log mesh-name hints; it does not modify the map, and its matches still require visual and navigation validation. Run it as a standalone commandlet after closing the editor. The classifier's offline regressions run with `python -I -m unittest discover -s Scripts/tests -v`. Label its actor "Rat", never "Fenrus" (see `docs/residents/README.md`).
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

## Update 2026-10-11: verified state

- MetaHuman optional content installed; `Scripts/Create-MetaHumans.py` (`-RenderOffscreen`) applied body, skin and eyes to `MHC_Aster` and `MHC_Innkeeper` and saved them. `can_build_meta_human` is still False for both: they need `request_auto_rigging` then `build_meta_human`, which calls Epic cloud services (login required). Not attempted yet, so no assembled MetaHuman exists and no resident pawn uses one.
- Raven: scratch build succeeded; `CaptiveSky2.Agent.RavenPerch`, `RavenFlight`, `RavenShelterCandidateAudit` pass (NullRHI, so the mesh was not seen on screen).
- Rat: `CaptiveSky2.Agent.RatAnimationAssets` and `RatLocomotionAnimation` pass (the Rat controller drives the Fox clips on `SK_Rat_01` via `BP_Rat_Placeholder`). A two-frame render of the placeholder showed only subtle idle motion. No rat is placed in the Island map; placement belongs with the Rat hollow work.

## Update 2026-10-11 (later): MetaHumans built

- The headless Auto-Rig asserted because it needs the MetaHuman Character editor open on the asset (and an Epic sign-in). Working sequence in one GUI editor session on the scratch project: open the asset (`AssetEditorSubsystem.open_editor_for_assets`), `request_auto_rigging` (one asset at a time; ~13 s), `request_texture_sources`, `build_meta_human` with `absolute_build_path=/Game/Characters/MetaHumans/Built/<Name>`, then `EditorLoadingAndSavingUtils.save_dirty_packages`. Both `MHC_Aster` and `MHC_Innkeeper` built and saved (`BP_MHC_*`, body/face meshes, baked textures under `Built/`).
- `Scripts/Create-MetaHumanBodies.py` makes `/Game/Agents/BP_Aster_MetaHuman` and `/Game/Agents/BP_Innkeeper_MetaHuman` (AutonomousAgentCharacter clones of `BP_Agent_Placeholder` with the MetaHuman body mesh and an attached Face component). A real-RHI capture of the Innkeeper body shows the body and head with skin; the face was backlit, no idle animation, and the characters are in the default underwear/bald state (no hair or clothing yet).
- Not wired: `IslandInnkeeperSubsystem::BodyClass` and the Aster spawn still point at `BP_Agent_Placeholder`. Content/ is gitignored, so pointing a committed default at a local-only asset would break other checkouts; switch it deliberately.

### In-game check (2026-10-11, scratch project, `-game /Game/Maps/Island -NullRHI`, 90 s)
- `LogRavenAgentAI: Raven appearance on BP_Raven_Placeholder_C_0: mesh=/Game/Characters/Creatures/Raven/SK_Raven_01 ... visible=yes` (the controller now selects the purpose-built raven).
- The Innkeeper spawned with `SKM_MHC_Innkeeper_BodyMesh` loaded (`Spawned Island innkeeper ... Agent_Innkeeper_01`). NullRHI, so nothing was seen on screen.
- Aster remains a map-placed actor on the placeholder body, and no rat actor is in the Island map.

## Update: rat spawns in the Island (2026-10-11)

`UIslandRatSubsystem` (Game/PIE worlds, Island map only) spawns `/Game/Agents/BP_Rat_Placeholder` as `Agent_Rat_01` at the nearest navigable point 700-1200 cm from the Inn landmark, forcing `ARatAgentAIController` (the Blueprint's controller class fell back to the generic one). Verified in a `-game -NullRHI` run: `Rat locomotion on BP_Rat_Placeholder_C_0: idle=ANIM_Fox_IdleBreathe ...` and `Spawned Island rat with stable identity Agent_Rat_01 ... (controller RatAgentAIController_0, mesh SK_Rat_01)`. The body is local Content, so without it the subsystem is a no-op. Idle animation is assigned by the controller but not yet seen in a rendered frame. Temporary stand-in until the rat hollow anchor lands.

## Update: rat placed in Island.umap (2026-10-11)

Per the user's go-ahead, `BP_Rat_Placeholder` is placed in `Content/Maps/Island.umap` (local-only, gitignored) as actor label `Rat` near the Inn at about (-99386, 103180, 2652), with `Memory.AgentId=Agent_Rat_01`, `DisplayName=Rat`, tag `Rat`, and the instance's `AIControllerClass` set to `RatAgentAIController` (the Blueprint default resolves to the generic controller). In a `-game -NullRHI` run the placed rat is possessed (`Rat locomotion on BP_Rat_Placeholder_C_0 ...`), decides and moves (`RatAgentAIController_0 decided: "Circle toward the innkeeper"`), and `UIslandRatSubsystem` does not spawn a duplicate. Fresh checkouts without the local map edit still get the subsystem's runtime rat.

## In-game visual review follow-up (2026-10-11)

- The first low-angle, bounded real-RHI capture placed the camera near the Rat but dense ground cover hid the body completely; the captured frame also showed `Preparing Shaders`, so it is not a settled visual check.
- A second isolated capture was configured for a steep overhead view and capped at 120 seconds of play and one model request, but the game never reached its world-ready marker within the 600-second startup limit. The wrapper terminated only its launched Game process tree. No second screenshot or runtime agency observation was produced; the Island map was not saved.
- The separate texture-pool audit, including the later settled runtime captures and remaining limits, is documented in `docs/findings/2026-10-11-texture-streaming-pool.md`.
