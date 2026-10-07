# Tideglass Dragonflies

Three wild dragonflies patrol close to the Tideglass shore from 06:00 until dusk. Each has a slim three-part body, four independently fluttering leaf-shaped wings, and one of three stable moss-green, blue-green, or copper color morphs. The collisionless wings are generated as small, double-sided surfaces with a swept outline; the segmented body remains a lightweight engine-sphere stand-in. They fly in small local loops, drift gently with local wind, slow in heavy rain, and steer around solid world geometry. A dragonfly near a natural wind- or rain-made pool ripple may briefly dip toward the water, then return to its patrol. This response is short, cooldown-limited, and does not apply to visitor-made ripples.

Residents can notice the nearest visible dragonfly within 18 metres. If one is close and in clear view, Aster or the raven may choose `Interact` with target `TideglassDragonfly` to watch; it darts a short way off and resumes its pool-side flight. This is a brief, reversible response. Dragonflies are never movement targets, companions, collectables, or saved entities, and observation makes no model request or persistent world change.

`AIslandWeather::RefreshNightEcology` owns their bounded population alongside the existing daylight crabs and minnows. Repeated refreshes reuse the three actors; dusk removes them, and the next daylight refresh creates a new small population. Their collision is disabled and they cast no shadows, so they cannot obstruct residents, the raven, or navigation.

## UE 5.8.3 validation (2026-10-04)

The isolated `Codex_UnderstoryVerify_20261002` editor target compiled and linked with the new dragonfly class, weather lifecycle, interaction resolver, resident awareness, and ecology assertions. The focused `CaptiveSky2.Agent.NightEcology` automation passed, covering the three-actor cap, daily emergence/dusk removal and dawn return, three distinct colors, collisionless body parts and wings, bounded wind/rain response, resident interaction routing, quiet startle-and-return behavior, and all pre-existing firefly, crab, and foliage checks. Build log: [UE 5.8.3 build](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_TideglassDragonfly_20261004_Build.log); test log: [NightEcology automation](../../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_TideglassDragonfly_NightEcology_20261004.log). This automation used a no-render fixture; in-world visual and performance screening remain to be done. No primary Island map or Content asset was modified.

## Fauna asset candidate (2026-10-04)

The Fab [Animal Variety Pack](https://www.fab.com/listings/2dd7964c-a601-4264-a53d-465dcae1644c) is currently listed as free and includes animated stag, doe, pig, and wolf assets. The connected browser was signed out, so no Fab library claim was made. A local copy already exists under ignored `Content/AnimalVarietyPack/` (copied from neighboring UE projects and separately documented in [animal-variety-pack.md](animal-variety-pack.md)); its assets are being checked for UE 5.8.3 use. The existing dragonflies remain their own small shore species. Any added wildlife should keep a bounded habitat/population, have its own unobtrusive routine, and remain perceivable rather than guaranteed to interact.

## Generated wing silhouette (2026-10-07)

The four placeholder ellipsoids are now narrow, swept procedural wings. Each is
a small two-sided, collisionless surface with a tapered outline and the same
stable morph color as before. Population, patrol, resident/raven responses,
body meshes, and timing are unchanged. Geometry is built once during actor
construction rather than every tick; the wingbeat only changes component
rotation. The UE 5.8.3 editor build succeeded and `CaptiveSky2.Agent.NightEcology`
passed, including finite tip/bounds, generated section, collision/navigation,
and existing interaction assertions. The matched real-RHI daylight preview
produced exactly three transient dragonflies and passed `CaptiveSky2.Visual.Viewpoints`
with no world-state writes. The wide Tideglass view cannot establish whether
the wing outline is readable in ordinary play or quantify its frame-time cost;
see [the capture finding](../findings/2026-10-07-dragonfly-silhouette.md).
