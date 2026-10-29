# Runtime woodland mix preview (2026-10-29)

## Preview conditions

Opened the Island through the UE 5.8.3 editor and used its standalone Play
preview. Disabled resident agent thinking for this inspection with
`CaptiveSky.Agent.DisableThinking 1`; the session's built-in 1,800-second and
120-request limits remained enabled. Stopped the preview manually after
194.2 real seconds. The end-of-session log reports zero model requests and
zero requests still in flight. No map or Content assets were saved or changed.

## Runtime woodland evidence

The game log reports 3,600 broadleaf trees across 381 of 500 groves under the
fixed tree budget: 900 Norway maple, 900 European aspen, 900 European beech,
900 black alder, 39 birch coppice, and 57 hazel coppice instances. This
confirms the runtime generator is not limited to one tree species. The wider
woodland report also counted 14,430 trees (8,745 saplings), 15,036 broadleaf
shrubs, and 1,491 flowering rhododendrons after 52,249 bounded traces.

The preview began at night. In the captured view, foreground understory was
dense while the mixed canopy read mostly as dark silhouettes; this is not a
useful daylight check of species readability or whether the distribution
looks natural. Species counts alone do not establish visual quality.

## Rendering and performance limits

The same runtime log contains material warnings that several instanced plant
materials lack the `InstancedStaticMeshes` usage permutation and will fall
back to Unreal's Default Material. Warnings cover Birch Coppice, Hazel
Coppice, multiple Rhododendron instances, and some Marsh Marigold and Yellow
Flag Iris materials. It also reports
`M_Island_Textured_Wet: Requesting an invalid TextureIndex! (21 / 20)`.
These warnings can undermine both species identity and visual variety even
when the generator selects different species. The existing plant-material
audit documents safe repairs for graph-valid roots and why the broken
Rhododendron master must not be toggled blindly:
[plant instanced-material usage repair](2026-10-07-plant-instanced-material-usage.md),
[Rhododendron fallback investigation](2026-10-08-rhododendron-material-fallback.md).

This short pass did not yield a trustworthy FPS or graphics-memory sample;
`stat fps` and `stat streaming` were not visible in the preview, and no
repeatable VRAM reading was captured. It also did not revalidate water
collision. Performance, texture-budget pressure, daylight species
readability, and walking-on-water remain open validation items rather than
verified fixes. A focused daylight capture and a repeatable FPS/VRAM sample
should come before claiming a performance or collision improvement.

## Next step

Coordinate a follow-up for the specific warned material roots and the wet
landscape texture-index issue. Because project Content is shared binary asset
state and the Rhododendron master has known graph defects, inspect the exact
affected roots and agree on scope before saving asset changes. Then capture a
daylight runtime view and measure frame time and graphics memory in the same
repeatable play route.

## Daylight PIE overlay observation

On a later observation of the open daylight PIE viewport, Unreal displayed
55.76 FPS and 17.93 ms/frame with the streaming-stat overlay visible. This is
above the project's 30-FPS target for that instant, but the screen also showed
`PROFILING WITH ALL LOGGING ON!`, and no timed sample or peak graphics-memory
reading was collected. Treat it as a useful indication, not a controlled
performance pass; repeat with logging/profiling overhead understood and a
recorded frame-time/VRAM interval.
