# Broadleaf mid-distance visibility trial — 2026-10-28

The Island already has a fixed budget of 3,600 broadleaf trees split across
Norway maple, European aspen, European beech, and black alder. The source was
placing them in hundreds of groves, but the first three skeletal species were
fully culled by 12,500 cm while spruce remained visible to 55,000 cm. The
already-imported Black Alder has a readable leafy crown in its asset preview,
so extending that fourth species into the middle distance is a low-count way to
break the repeated spruce-only silhouette. This is a visibility hypothesis,
not a proven sole cause.

This reversible trial keeps the same populations and placements, leaves maple,
aspen, and beech unchanged at 8,000/12,500 cm, and moves only Black Alder from
8,000/12,500 cm to 12,000/22,000 cm. It should expose this distinct species
through an additional ~95 m of midground while keeping it well inside the
spruce horizon. It adds no instances, collision, or navigation data, but may
increase rendering and texture-streaming cost because previously culled alder
instances can now be visible. Existing tests that assert the first three
species' distances remain valid and were not edited.

The main `CaptiveSky_2Editor` target compiled and linked after this isolated
source change. On 2026-10-29 the user-approved restart reopened the main project
on the Island in UE 5.8.3. A first in-editor automation run used an older
test-module assertion: it expected Alder's previous 8,000/12,500 cm range while
the live component reported 12,000/22,000 cm. Live Coding then compiled and
linked 14 actions successfully (56.41 seconds), including
`IslandWeatherBroadleafTests.cpp`. The rerun of
`CaptiveSky2.Agent.IslandWeather` passed both `BroadleafDiversity` and
`BroadleafMixing` at 13:04 local. This verifies the current cull-bound
assertions and deterministic mixing helpers, not the in-world appearance or
rendering cost. The editor viewport remains the authored non-PIE scene; no
matched runtime view, 30-FPS sample, VRAM sample, or texture-budget measurement
was collected. Do not treat the new range as approved until the same fixed
Island view is compared against the old range with a sustained 30-FPS floor and
graphics-memory headroom. Revert the Alder cull values if either guard
regresses; do not compensate by increasing or decreasing the fixed tree budget
during that comparison.

An unattended `UnrealEditor-Cmd` attempt to run `CaptiveSky2.Agent.IslandWeather`
returned exit code 1 before creating a fresh project log or reporting a test
result. The captured output ended after an all-platform AutoSDK report that
listed Win64 as valid and missing LinuxArm64/VisionOS SDK versions. The local
`ValidatePlatformsMode` source returns success even when some platforms are
invalid, so the output does not establish that those optional SDKs caused the
exit. That headless failure remains unexplained, but it is no longer the only
test evidence: the live-editor automation passed after a successful Live Coding
compile as described above.
