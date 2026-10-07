# Landmark visual audit (2026-10-01)

## Result

A read-only Unreal Editor Python inspection loaded `/Game/Maps/Island` and
enumerated 169 level actors. The visible pale forms around the named landmarks
are blockout primitives, not authored stone sculptures or accidental debug
markers:

| Landmark | Level actors | Mesh and material |
| --- | --- | --- |
| Wind Arch | `WindArch_Pillar_A`, `WindArch_Pillar_B`, `WindArch_Beam` | Three `/Engine/BasicShapes/Cube` meshes using `/Engine/BasicShapes/BasicShapeMaterial` |
| Listening Stones | `ListeningStone_A`, `ListeningStone_B`, `ListeningStone_C` | Three `/Engine/BasicShapes/Cube` meshes using `/Engine/BasicShapes/BasicShapeMaterial` |
| Tideglass Pool | `TideglassPool` | `/Engine/BasicShapes/Sphere` using `/Engine/BasicShapes/BasicShapeMaterial` |

The bounds support what the captures suggested: the Wind Arch is a pair of
upright rectangular pillars (half-heights 333 and 299 cm) with a long narrow
beam (half-length 380 cm); each Listening Stone is a narrow upright block
(half-heights 112–151 cm); the pool is a shallow sphere (half-extents
160×160×9 cm). These landmarks need authored silhouettes and materials, not a
global landscape-material adjustment.

The inn is also assembled mostly from `/Engine/BasicShapes/Cube` meshes, but
many have authored `/Game/Inn/Materials/MI_Inn_Plaster` or
`MI_Inn_Timber` materials. A few interior/floor pieces still use the Starter
Content oak material. The nearby spruce trees are authored assets from
`/Game/PN_interactiveSpruceForest`, whereas the natural ledge and roost foot
stones use Starter Content's `SM_Rock` and `M_Rock`.

## Recommended art pass

1. Keep the current landmark locations and gameplay targets fixed.
2. Replace only the landmark render meshes with more organic, weathered forms
   while retaining the existing collision / interaction targets: carved,
   mossed standing stones; a wind-worn stone arch with a visibly open center;
   and a deliberately shallow, readable reflecting pool.
3. Use the existing foliage and rock asset families as palette references;
   avoid introducing a new marketplace dependency until its licensing and
   visual fit are confirmed.
4. Capture the same viewpoints in daylight and at golden hour/night, then
   verify the landmark interactions still route to the original targets.

No `.umap` or `.uasset` files were changed. They are gitignored and shared
outside the reviewable source workflow; any replacement should be coordinated
as a separate Content/map-art pass before edits.

## Reproduction and validation

The inventory script is [Scripts/Inspect-IslandLandmarkGeometry.py](../../Scripts/Inspect-IslandLandmarkGeometry.py).
It ran against UE 5.8 with the Python scripting plugin using
`UnrealEditor-Cmd.exe`, `-NullRHI`, and `-DDC-ForceMemoryCache`; the latter is
needed in this restricted environment because the configured user DDC is
read-only and the local Zen service cannot be updated here. The script logged
four landmark anchors and nearby component bounds/materials, then the editor
exited with code 0. It emitted an Editor Scripting Utilities deprecation
warning for `EditorLevelLibrary.get_all_level_actors`; this did not affect the
inventory and can be updated when the script is next extended.

Two earlier launches in this inspection attempt crashed before the script ran:
PowerShell argument quoting caused Unreal to ignore the local DDC override,
then its default `Installed` cache graph failed because it had no writable
node. Those are inspection-launch failures, not project/runtime exceptions.

## Transient rock-silhouette preview (2026-10-01)

`CaptiveSky2.Visual.Viewpoints` accepts `-ViewpointLandmarkRockPreview` to test the six Wind Arch and Listening Stones render silhouettes with the existing Starter Content `SM_Rock` and `M_Rock`. It adds collisionless, navigation-free child components at the blockout transforms and hides only the original render components; those original meshes and collision remain in place. A latent cleanup command removes all six preview components and restores the prior visibility, including after a capture failure. Preview scale is fitted from each blockout mesh's local bounds to avoid the oversized first attempt. The flag changes neither the saved map nor any asset, and is an art-direction comparison only—not a gameplay or final-mesh approval.

The combined 17:00 real-RHI capture used `-ViewpointLandmarkRockPreview -ViewpointGroundCover` and passed `CaptiveSky2.Visual.Viewpoints`. The rocks now fit the existing blockout dimensions better, though remain dark and backlit; the beam is still partly clipped at the top of this viewpoint. The added grass makes the approach less barren. Capture: `Saved/CompileScratch/Codex_InnGroundCover_20261001/Project/Saved/Viewpoints/2026-10-01_161310_h17.0/04_WindArchOverlook.png`; baseline: `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-10-01_040746_h17.0/04_WindArchOverlook.png`. If the rock forms are promising, coordinate a separate authored Content/map-art pass for intentional scaling, distinct carved shapes, and a readable arch opening; do not promote stretched Starter Content rocks as the finished landmark art.

## Early Listening Stones cairn pass (2026-10-07)

The transient Game/PIE Listening Stones presentation now builds each tall, narrow blockout proxy from several overlapping `SM_Rock` instances with uniform scale, deterministic yaw, and slight layer offsets, rather than stretching one mesh vertically. The original map actors remain authoritative for collision, navigation, and landmark location; only their Game/PIE render is replaced. The UE 5.8.3 editor target built successfully and `CaptiveSky2.Agent.ListeningStonePresentation` passed, including natural aspect-ratio and proxy-bounds assertions: [`ListeningStoneStack.log`](../../Saved/Playtests/Codex_StoneCairn_20261007/ListeningStoneStack.log).

This is still an unreviewed visual prototype, not final art. A single bounded 45-second Game capture was attempted with thinking disabled, zero effective model calls, an isolated data root, and a 600-second startup guard, but Unreal spun in Turnkey `ValidatePlatforms` without reaching the Island world-ready marker; that exact launched Game process was stopped. No screenshot was produced, so judge the stacked silhouette in a real-RHI runtime capture before accepting or tuning it.

## Wind Arch pillar stack refit (2026-10-07)

The [existing real-RHI Wind Arch frame](../../Playtests/Codex_ForwardLightDusk_20261007/Screenshots/008_Wind_Arch_Overlook.png) shows each pillar as a vertically stretched chain of large rock forms. The Game/PIE presentation now derives a stack count independently from each saved pillar's height and footprint, using smaller uniform-scale rock forms with slight deterministic yaw, depth, and size variation. The arch's wide center remains open; the five lintel pieces, saved transforms, collision, navigation, and map data are unchanged. No `Content/` assets were edited.

The UE 5.8.3 editor target built successfully and `CaptiveSky2.Agent.WindArchPresentation` passed, including checks for natural rock proportions, proxy-envelope fit, distinct left/right pillars, five lintel stones, and the open center: [`Codex_WindArchStack_20261007.log`](../../Saved/Logs/Codex_WindArchStack_20261007.log). The stacked runtime silhouette has not yet been visually captured; the linked frame is the pre-change evidence. A matched Game/PIE view remains the art-direction check before treating this as finished.
