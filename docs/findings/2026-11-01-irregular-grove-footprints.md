# Break up repeated woodland grove footprints — 2026-11-01

## Change

The deterministic woodland scatter previously used the same inner and outer
radii for every grove. Even though individual trees were random and area-uniform,
the shared geometry could make adjacent plant communities read like repeated
circular beds.

Each grove now samples a smooth, seeded 9,000 cm community field at its center
and scales the mature-tree, sapling, shrub, rhododendron, and coppice annuli by
the same restrained factor in the range 0.82–1.18. This makes neighboring grove
footprints vary gradually rather than repeat identically. The field is separate
from the placement random stream; per-grove instance targets, species shares,
tree-population limits, collision, navigation, and landmark clearances are
unchanged. Invalid positions resolve to the neutral scale.

The focused broadleaf regression now checks the scale bounds, seeded repeatable
results, non-finite fallback, and meaningful variation over representative
Island-scale coordinates.

## Validation and limits

The UE 5.8.3 scratch Editor target compiled and linked successfully (189 UBT
actions; 782 seconds), including `IslandWeather.cpp` and
`IslandWeatherBroadleafTests.cpp`. The automation test compiled but was not
executed: the scratch project's known NullRHI pre-initialization crash remains
unresolved, and the open D: editor has a different project module. No PIE or
Game session was started because current GPU/system-memory headroom has not been
re-established above the project's rendered-play safety gate. This is a
count-neutral placement improvement, not evidence of a more natural rendered
result or unchanged frame time.

Next, compare the same daylight Tideglass-to-woodland route against the prior
seeded baseline, checking grove-edge repetition, close/mid/far species
readability, frame-time p95, whole-device VRAM, and physical-RAM headroom. Keep
the change only if the woodland reads less mechanically without breaking the
30-FPS floor or memory budget.
