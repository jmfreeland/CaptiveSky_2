# Foliage sway index compaction — 2026-10-17

The CPU foliage-sway pass gathers candidate instance indices from nearby spatial
cells. Its old path sorted those candidates, removed duplicates and out-of-radius
entries one at a time with `RemoveAt`, then concatenated, sorted and deduplicated
the previous/current sets before doing a binary search for each update. Repeated
array shifting made the collection work quadratic in the number of gathered
indices in the worst case.

The new path keeps the same cells, focus points, radius, base transforms,
previous/current reset behavior and per-instance wind/resident bends. It sorts
the gathered candidates once, compacts duplicates in one pass, filters outside
the focus circle with stable `RemoveAll`, merges the two sorted unique sets in
one pass, and walks current membership linearly while applying the same
transforms. The full foliage population and authored transforms are unchanged.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully
(35 compile actions), including `IslandWeather.cpp` and `IslandEcologyTests.cpp`.
The focused `CaptiveSky2.Agent.GroundCover` test did not run: the disposable
`UnrealEditor-Cmd` process crashed during startup with a null-address access
violation at `SecondsSinceStart=0`. Its Core-only `PCallStackHash`
`F405610C114D120D4B6F5ABA901EC469AC9FAA26` matches the earlier project-opening
startup failure in
`docs/worldbuilding-ideas.md`; the crash occurred before the automation test
began. The launch used NullRHI, disabled resident thinking, and allowed zero
model requests. No FPS improvement or runtime behavior pass is claimed.

The GroundCover automation now asserts exact sorted-unique compaction and
previous/current set-union outputs. Next, resolve the project-startup crash,
run `CaptiveSky2.Agent.GroundCover`, then compare warmed full-population Game
captures and a trace at the same view before attributing any frame-time gain.
Do not change sway defaults or reduce vegetation on this result alone.
