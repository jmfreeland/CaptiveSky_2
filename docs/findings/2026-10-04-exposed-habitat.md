# Wind Arch: first exposed ground-cover habitat

## Intent

Today's highlight captures showed dense nearby broadleaf cover against thin tree silhouettes and bare middle-distance terrain. Increasing visibility distances alone previously failed repeatable performance validation. This milestone changes composition, not instance density or visibility ranges: Wind Arch should read as a grass-dominated exposed place rather than another woodland verge.

## Rules implemented

- The closest Wind Arch landmark owns an exposed core out to 50 metres, with a 15 metre transition back to the existing meadow mix.
- A nearer non-exposed landmark or inn entrance keeps its existing mix; ties favour that non-exposed habitat.
- Existing deterministic 7 metre botanical patches select from the same three grass meshes. Exposure progressively replaces broadleaf patches, not individual random tufts. Fully exposed patches contain grass only.
- The immediate Wind Arch verge still substitutes a bounded share of Festuca. Terrain, slope, sea-level checks, open landmark centres, collision/navigation exclusion, instance budgets, culling and wind baselines are unchanged.
- No map/material assets, canonical world state, resident memories, play/request limits or LLM settings are modified.

This is presentation scaffolding, not a canonical ecology simulation. It does not yet make construction alter habitats, add a species lifecycle, fix distant tree silhouettes or solve the whole Tideglass-to-mountain composition.

## Validation

UE 5.8.3 Development Editor build succeeded. The focused `CaptiveSky2.Agent.GroundCover` test passed in the isolated scratch project (`Codex_ExposedHabitatGroundCoverVerified_20261004.log`).

The fixture covers exposed core/transition/end distances, absent anchors, competing landmark ownership, ties, bounded and repeatable species selection, all three grass forms, actual absence of broadleaf placements in the Wind Arch verge, and the pre-existing placement/sway/collision/navigation invariants. A second materialization of the same fixture without Wind Arch confirms the other landmark placement and broadleaf counts remain unchanged.

Initial test runs failed because an old global broadleaf-ratio assertion assumed every landmark used the same mix. A subsequent attempt to apply that ratio only to the other small fixture verges also failed: those verges already sample uneven botanical patches. The final test uses direct baseline count comparisons instead of relaxing a ratio until it passes. The default species-distribution test remains unchanged.

Golden-hour capture succeeded (`Codex_ExposedHabitatGolden_20261004.log`, images under scratch `Saved/Viewpoints/2026-10-04_145148_h17.0`). The arch foreground has less broadleaf cover and more open grass/Festuca tufts; woodland shrubs remain behind it. This is a modest local composition change, not a finished landscape.

Ground-level golden-hour p95 throughput ranged from 33.91 to 39.33 FPS; Wind Arch was 39.33 FPS, versus 56.97 in today's earlier capture. The elevated survey remained below the floor (18.76 versus 13.15 earlier). These are observations, **not a controlled performance comparison**: `Island.umap` was modified at 14:50:35 between captures, and a separate Claude offscreen capture was subsequently confirmed live with shader workers. The actual-map ground-cover totals were 1,780,021 versus 1,780,052 earlier; tree/shrub/flower counts remained unchanged. Candidate budgets and trace counts are unchanged, but exact placement equality is proven only in the focused fixture.

The first 11:00 Tideglass follow-up **failed** (`Codex_ExposedHabitatTideglass_20261004.log`): broad view 7.99 FPS p95, 46/50 valid intervals, 1.39 FPS wall throughput; detail view 33.53 FPS p95. The log includes several multi-second stalls, including a 20.81 second delta. Concurrent process 26952 was positively identified as the other agent's `Claude_Props` viewpoint capture, not an abandoned process to kill. That is a measurement confound, not grounds to waive the failed gate. Repeat after contention subsides before accepting performance.

Capture logs also report missing `/PlantFactoryPlugin/BillboardMappedNormals` and `/PlantFactoryPlugin/StaticBillboard` dependencies for existing plant billboard materials. Mesh/material-slot resolution tests do not establish complete material dependency health; audit this before claiming imported vegetation is fully healthy.

The repeat passed (`Codex_ExposedHabitatTideglassRepeat_20261004.log`): broad view 35.91 FPS p95 / 43.11 wall FPS; detail view 46.43 FPS p95 / 52.42 wall FPS; both 50/50 valid intervals. The map's SHA-256 was identical before and after (`FBB1FA7B35C2FE6AD74EC7326877DD512E7C94C020D56E20ACED441BDB58BA83`). Captures are in scratch `Saved/Viewpoints/2026-10-04_145819_h11.0`. Another editor process appeared during this repeat, so it demonstrates a passing measurement, not fully isolated benchmarking or guaranteed sustained play performance. Retain both runs; future performance work needs a shared exclusive capture window.

No gameplay or model calls were used for these captures.

## Next

Extend composition toward maintained inn circulation, meadow masses and woodland-edge silhouettes. First audit missing billboard dependencies and obtain exclusive warmed render/GPU measurements. Preserve the 30 FPS broad-view requirement; a successful capture alone is not proof that all views satisfy it. The high survey remains an explicit unresolved performance issue.
