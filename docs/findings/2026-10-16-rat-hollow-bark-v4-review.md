# Rat hollow Bark V4 review (2026-10-16)

## Result

Bark V4 is a promising source-art candidate, but it is **not approved for Island placement**. The Blender source preview (`Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV4/Preview-Entrance.png`) reads as a weathered fallen trunk with a visible entrance, moss patches, and a straw bed. The imported Unreal capture is unusable for judging final appearance: the mesh renders with checkerboard material patterns and severe black shadow regions (`Saved/Playtests/Codex_RatHollow_BarkV4Review_20261016/RatHollow_BarkV4_FullScene_Warm.png`).

This may be a preview/material compatibility issue rather than a bad mesh. The import report contains no validation errors, records four material assignments and nine test-local textures, and the warm preview log confirms the actor's four expected material slots. The capture still shows checker patterns, so successful assignment alone does not establish that the materials render correctly.

## Import and safety

- Imported only into `/Game/Generated/ComfyBlender/RatHollow_BarkV4_Test_20261016`.
- To avoid replacing shared library textures, the review import used copies of the FBX and all nine textures under `Saved/CompileScratch/Codex_RatHollowV4Source/` and a temporary local-texture manifest. The import report says `shared_library_textures: []`.
- Mesh report: approximately 270.8 × 145.2 × 141.9 cm, 2,072 triangles, five box collision elements, four material slots, no validation errors.
- The review actor and camera were transient. The Island map was not saved, `Agents/Agent_Rat_01/places.json` remains without a home, and no canonical site was approved.
- UnrealEditor PID 22688 was absent from the process list when checked after the user asked that it be left running. No process was terminated and the interactive editor was not relaunched.

## Material diagnostic follow-up

The preview script now logs the actual texture references on each material's texture-sample nodes as well as the base-color input. Its Python syntax check passed. A non-saving Unreal commandlet attempt did not reach the script: the project-opening path crashed at startup with a null access violation at `SecondsSinceStart=0`, `PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`, matching the project-opening crash documented on 2026-10-13. The command line in the crash-side log is correct; the requested project log was not created. This provides no new evidence about the material references, so the checkerboard remains unresolved. Do not repeat this launch until the project-startup environment changes or the crash can be symbolized.

Crash evidence: `Saved/Crashes/_0000/Codex_RatHollowTextureAudit_20261016.log`, `Saved/Crashes/_0000/CaptiveSky_2.log`, and `Saved/Crashes/_0000/CrashContext.runtime-xml`.

## Evidence

- Source preview: `Saved/CaptiveSky/ComfyBlender/RatHollow_BarkV4/Preview-Entrance.png`.
- Unreal capture: `Saved/Playtests/Codex_RatHollow_BarkV4Review_20261016/RatHollow_BarkV4_FullScene_Warm.png` (an earlier cold capture is in the same folder).
- Import report: `Saved/CompileScratch/Codex_RatHollowV4Source/RatHollow_BarkV4/unreal_import.json`.
- Import and preview logs: `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_RatHollowBarkV4Import_20261016.log` and `Codex_RatHollowBarkV4Preview_Warm_20261016.log`.

## Next safe step

Once the project-startup crash has been diagnosed, inspect the imported material texture-sample references and material compile/preview path before changing source art. Then repeat a normal editor/game viewport review without saving the Island. Separately, choose and approve an exact concealed fallen-wood site that satisfies the existing canopy and habitat requirements before adding a Rat home record or placing the asset in the world.
