# Island highlight review (2026-10-05)

This review started from saved 2026-10-04 highlights and now includes a bounded runtime capture of the first Wind Arch visual pass. The 2026-10-05 previews ran from the isolated scratch project with agent thinking disabled and a 50-second maximum play time; world data was kept under a separate scratch root.

## Strongest current composition

The [Wind Arch raven-at-sunset frame](../../Saved/Highlights/2026-10-04/1926/Wind-Arch-Raven-at-Sunset.png) remains the strongest resident-focused composition. A new [golden-hour stonework capture](../../Saved/Highlights/2026-10-05/WindArch-Stonework-Golden-Hour.png) shows a full, rough-rock lintel and irregular pillar stacks in the saved Island. This is a transient Game/PIE presentation: 15 existing Starter Content rock-mesh instances replace the three cube visuals, match the two pillars' unequal saved heights, and restore each proxy's previous visibility on teardown. The original proxies retain collision; the replacement is collisionless and cannot affect navigation. No map or Content asset was edited.

The full-arch diagnostic view is a useful shape check, not yet a final resident highlight: the pale vertical forms in the opening are `ListeningStone_A/B/C`, 13–16 m beyond the arch. The short normal-route view also clips the crown. The isolated capture had no Raven in frame, so it does not replace the resident-focused sunset image. `CaptiveSky2.Agent.WindArchPresentation` passed in the UE 5.8.3 scratch project, checking saved-map proxy recognition, all 15 instances, proxy-specific pillar heights, collision/nav invariants, and visibility restoration.

The three Listening Stones now have a matching reversible Game/PIE pass using the same existing `SM_Rock` family. Each render instance is fitted to its proxy's saved bounds; the cubes remain in place as the collision/nav authority, preserve their prior visibility on teardown, and are only hidden in-game. When an agent or player rings the Stones, a short wind-modulated blue-white light response joins the existing 2.8-second chime, then switches fully off. `CaptiveSky2.Agent.ListeningStonePresentation` passed against the saved Island and a synthetic game world, including proxy dimensions (half-heights 112, 136, and 152 cm), collision/nav invariants, initial visibility restoration, and finite chime response. This gives the blockouts intentional silhouettes without editing the map or assets. Claude's separate lichen treatment remains in flight; coordinate it with these presentation lights after that work lands.

The [Tideglass wet-edge dusk frame](../../Saved/Highlights/2026-10-04/1942/Tideglass-Wet-Edge-at-Dusk.png) has appealing cloud color and useful near-water vegetation, but the pool still reads as a flat prototype surface. Bright spherical accents pull attention away from the shoreline, and heavy foreground cover obscures the water edge.

The [coastal overview](../../Saved/Highlights/2026-10-04/154352/Coastal-Overview.png) gives a useful scale check: it exposes broad bare ground between the shore and the woodland, while the distant tree line is sparse and trunk-heavy. The [inn-at-golden-hour frame](../../Saved/Highlights/2026-10-04/154352/Inn-at-Golden-Hour.png) similarly shows how the pale proxy architecture dominates otherwise promising warm light.

## Next visual pass

1. Align the in-game monolith chime lights with the in-flight lichen treatment, then adjust the Wind Arch approach framing so its rough lintel reads without losing the raven and nearby resident. Decide whether the Starter Content rocks remain a temporary stand-in or earn an authored landmark asset pass.
2. Give Tideglass a convincing shallow-water/shoreline transition and remove or resize the distracting bright accents after identifying their source. Preserve a visible open-water shape rather than hiding it under vegetation.
3. Address the coastal midground with grouped habitat masses and a readable circulation gap, not a uniform increase in instance count. The close Tideglass vegetation view has already failed the 30 FPS p95 screen, so broadening foliage coverage must be paired with the performance target.

For the next highlight set, recapture the same Wind Arch and Tideglass compositions at matched framing after one visual change. Keep images separate from performance evidence: screenshots establish composition, while the real-time and p95 measurements remain distinct gates.

## Bounded full-route Game review (2026-10-05)

A standalone Game run captured the eight configured Island views at 1600×900,
17:00 Island time, with agent thinking and Python disabled, an isolated data
root, and hard limits of 60 real seconds / one model request. It ended normally
after 60.4 seconds with zero requests. All eight screenshots were produced;
logs confirm the safety cap and shutdown. The current main-project editor DLL
is older than `IslandWeather.cpp`, so this is a saved-map and art-direction
review, not validation of the pending source changes. Images:
[`001_Shore_Approach`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/001_Shore_Approach.png),
[`002_Inn_From_Path`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/002_Inn_From_Path.png),
[`003_Inn_Common_Room`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/003_Inn_Common_Room.png),
[`004_Inn_Guest_Book`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/004_Inn_Guest_Book.png),
[`005_Tideglass`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/005_Tideglass.png),
[`006_Tideglass_Ground_Detail`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/006_Tideglass_Ground_Detail.png),
[`007_Listening_Stones`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/007_Listening_Stones.png),
and [`008_Wind_Arch_Overlook`](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/008_Wind_Arch_Overlook.png).

The review confirms that the existing highlights remain stronger than this
route set. The broad Shore Approach reads as exposed brown ground with a sparse,
thin tree silhouette. The Tideglass ground-detail camera is overwhelmed by
close grass, while the pool's edge remains hard and is partly obscured. Several
bright spherical accents around the water pull focus; their runtime actor
source is not yet identified. The Inn and Wind Arch views still expose large
blockout geometry, and the interior cameras clip through the Inn structure.
These images are useful diagnostics, not highlight candidates.

A second capped Game run used the isolated 180 cm CPU-sway build, a separate
world-state root, no agent thinking, and the same 40-second / one-request
ceiling. It completed in 40.2 seconds with zero requests and retained 22 HISM
groups / 1,812,286 instances. Its first five views, including Tideglass, are
under `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Playtests/Codex_GrassSway180_Capture_20261005/Screenshots/`.
The single captured frame cannot establish whether WPO wind remains convincing
at this radius; the separately measured p95 is still 129.57 ms, above the
33.3 ms target. No production files, map, or Content assets were changed.

Next: keep the full habitat and composition, but treat the Tideglass wet-edge
and the exposed Shore Approach as separate art-directed zones rather than
increasing uniform density. First identify the spherical light/mesh accents
and improve the pool-to-ground transition; then frame a matched Tideglass view
with readable open water, a designed planted rim, and a clear path into the
meadow. Do not change `IslandWeather.*` until its current owner releases it;
screen the resulting view against the 30-FPS p95 goal separately from this
visual review.
