# Meadow flower pockets

Eight Fab flower species and one woodland bluebell clump now grow in small,
coherent pockets rather than as isolated single accents. Each accepted deterministic site keeps one
anchor bloom and attempts at most two same-species neighbors, at roughly 5.75 m
and 8.5 m from that anchor. The existing landscape-only ground trace, slope and
sea-level checks, landmark exclusions, grove clearances, minimum spacing,
species selection, and per-site attempt bounds still gate every bloom. Pockets
do not create collision, navigation, memory, persistent state, or model calls.

The current code tries at most 256 general meadow sites (the first 160 are
reserved for the ListeningStones annulus) plus eight Tideglass pollinator sites.
With three flowers per accepted site, the strict upper bound is 792 before
terrain and clearance rejections, plus the pre-existing sparse rhododendrons.
That is a small cap alongside the ground-cover population, not a general
increase to the grass budget. A deterministic offset regression checks the
pocket radii and minimum stem spacing.

The woodland bluebell mesh is used as the ninth deterministic patch variant,
not scattered as a separate population. Its HISM shares the existing
collisionless, navigation-free, culling, cleanup, and sway paths. Adding it
does not change candidate sites, per-site attempts, flower count ceiling,
ground-cover count, or model-request budget. The asset is currently stored in
the local generated-content review folder; a clean checkout needs the existing
ComfyBlender export/import step before the runtime reference can resolve.

## UE 5.8.3 validation (2026-10-10)

The current main-project editor target built successfully. Focused
`IslandWeather` and `GroundCover` automations passed with NullRHI, resident
thinking disabled, zero allowed model requests, and isolated world data. The
real saved-Island Game bootstrap then reached map BeginPlay and completed its
terrain scatter using a separate data root: 1,779,869 grass/ground-cover
instances and 990 Fab meadow flowers across the 512 candidate sites, including
21 flowers in the ListeningStones annulus and all eight species. Flower
placement used 1,085 bounded traces; the per-site maximum remains three. The
run reached its 60.1-second real-time cap with zero model requests and shut
down normally. It used NullRHI, so this verifies a complete bounded
simulation/bootstrap and the placement counts, but not rendered flower-group
readability or frame rate.

Evidence: [automation log](../../Saved/Logs/Codex_MeadowFlowerPockets_Automation_20261010.log)
and [bounded saved-Island Game log](../../Saved/Logs/Codex_MeadowFlowerPockets_Game_20261010.log).
The roughly 631-instance increase over the previous 359-flower baseline still
needs a matched rendered view and frame-time profile. The grouping intent and
placement bounds are verified; gameplay-scale readability and performance are
not yet verified.

## Woodland bluebell variant (2026-10-17)

The existing locally imported `WoodlandBluebells` clump is now a ninth
deterministic meadow-pocket variant. It uses the same HISM lifecycle, sway,
culling, collision and navigation policy as the eight Fab flower forms; it
does not add placement sites or raise the per-site or ground-cover limits.
The focused UE 5.8.3 `CaptiveSky2.Agent.MeadowFlowerSpecies` automation passed
against the isolated scratch build with NullRHI, resident thinking disabled,
and zero model requests. It confirmed the imported mesh resolves and the
bluebell component is non-colliding and excluded from navigation.

The broader `GroundCover` automation now passes in the isolated UE 5.8.3
scratch build. The tiny synthetic fixture checks low broadleaf visibility only
when a variant receives placements, while the large-grid species-selection
test covers world-scale variant diversity. Its NullRHI resident fixture marks
the synthetic character as recently rendered so the proximity-bending contract
is exercised without changing runtime visibility gating. This is not a
rendered quality or performance check. Evidence: [GroundCover automation log](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_GroundCoverReview_20261017b.log).
The source asset is in the ignored local review folder
`Content/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/`; other
checkouts need to run the existing ComfyBlender export/import pipeline before
the new species can render.

### Woodland-edge niche (2026-10-17)

Bluebell selection now follows the nearest accepted spruce-grove center. Its
deterministic affinity is strongest within 12 m and fades smoothly
to the original meadow mix by 38 m. Open meadow retains the existing nine-way
selection; at a grove center bluebells rise to 35% of already-budgeted flower
patches, with the other eight forms still represented. Members of each pocket
continue sharing one species, and the Tideglass pollinator ring keeps its
neutral mix. This changes neither the 256 general candidate sites, three-stem
pocket cap, trace ceiling, nor total placement budget.

UE 5.8.3 scratch build succeeded; `GroundCover` passed under NullRHI with
thinking disabled and zero model requests. Tests cover the distance falloff,
nearest grove, treeless meadow, deterministic patch choice, bounded bluebell
share, and preservation of the other flower forms. The fixture has no landscape,
so it does not execute saved-Island scatter; a real-RHI review is still needed
to confirm the new bluebell group reads naturally at woodland distance and
keeps the project above its 30 FPS target. Evidence: [GroundCover log](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_WoodlandBluebellNiche_Final_20261017.log).
