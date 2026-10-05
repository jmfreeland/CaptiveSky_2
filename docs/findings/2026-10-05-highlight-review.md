# Island highlight review (2026-10-05)

This review started from saved 2026-10-04 highlights and now includes a bounded runtime capture of the first Wind Arch visual pass. The 2026-10-05 previews ran from the isolated scratch project with agent thinking disabled and a 50-second maximum play time; world data was kept under a separate scratch root.

## Strongest current composition

The [Wind Arch raven-at-sunset frame](../../Saved/Highlights/2026-10-04/1926/Wind-Arch-Raven-at-Sunset.png) remains the strongest resident-focused composition. A new [golden-hour stonework capture](../../Saved/Highlights/2026-10-05/WindArch-Stonework-Golden-Hour.png) shows a full, rough-rock lintel and irregular pillar stacks in the saved Island. This is a transient Game/PIE presentation: 15 existing Starter Content rock-mesh instances replace the three cube visuals, match the two pillars' unequal saved heights, and restore each proxy's previous visibility on teardown. The original proxies retain collision; the replacement is collisionless and cannot affect navigation. No map or Content asset was edited.

The full-arch diagnostic view is a useful shape check, not yet a final resident highlight: the pale vertical forms in the opening are `ListeningStone_A/B/C`, 13–16 m beyond the arch, and all three still use `/Engine/BasicShapes/Cube.Cube`. The short normal-route view also clips the crown. The isolated capture had no Raven in frame, so it does not replace the resident-focused sunset image. `CaptiveSky2.Agent.WindArchPresentation` passed in the UE 5.8.3 scratch project, checking saved-map proxy recognition, all 15 instances, proxy-specific pillar heights, collision/nav invariants, and visibility restoration.

The [Tideglass wet-edge dusk frame](../../Saved/Highlights/2026-10-04/1942/Tideglass-Wet-Edge-at-Dusk.png) has appealing cloud color and useful near-water vegetation, but the pool still reads as a flat prototype surface. Bright spherical accents pull attention away from the shoreline, and heavy foreground cover obscures the water edge.

The [coastal overview](../../Saved/Highlights/2026-10-04/154352/Coastal-Overview.png) gives a useful scale check: it exposes broad bare ground between the shore and the woodland, while the distant tree line is sparse and trunk-heavy. The [inn-at-golden-hour frame](../../Saved/Highlights/2026-10-04/154352/Inn-at-Golden-Hour.png) similarly shows how the pale proxy architecture dominates otherwise promising warm light.

## Next visual pass

1. Turn `ListeningStone_A/B/C` into deliberate stone monoliths and align them with the in-flight luminous-lichen treatment; then adjust the Wind Arch approach framing so its rough lintel reads without losing the raven and nearby resident. Keep visual-only experiments reversible until the saved composition reads cleanly.
2. Give Tideglass a convincing shallow-water/shoreline transition and remove or resize the distracting bright accents after identifying their source. Preserve a visible open-water shape rather than hiding it under vegetation.
3. Address the coastal midground with grouped habitat masses and a readable circulation gap, not a uniform increase in instance count. The close Tideglass vegetation view has already failed the 30 FPS p95 screen, so broadening foliage coverage must be paired with the performance target.

For the next highlight set, recapture the same Wind Arch and Tideglass compositions at matched framing after one visual change. Keep images separate from performance evidence: screenshots establish composition, while the real-time and p95 measurements remain distinct gates.
