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

The main `CaptiveSky_2Editor` target compiled and linked successfully after
this isolated source change. The main editor cannot currently be reopened past
the missing Aura plugin prompt, so there is no matched visual, 30-FPS, VRAM, or
texture-budget result for this trial. Do not treat the new range as approved
until the same fixed Island view is compared against the old range with a
sustained 30-FPS floor and graphics-memory headroom. Revert the Alder cull
values if either guard regresses; do not compensate by increasing or decreasing
the fixed tree budget during that comparison.

An unattended `UnrealEditor-Cmd` attempt to run `CaptiveSky2.Agent.IslandWeather`
returned exit code 1 before creating a fresh project log or reporting a test
result. The captured output ended after an all-platform AutoSDK report that
listed Win64 as valid and missing LinuxArm64/VisionOS SDK versions. The local
`ValidatePlatformsMode` source returns success even when some platforms are
invalid, so the output does not establish that those optional SDKs caused the
exit. Treat the startup failure's cause as unknown; it is inconclusive about
the IslandWeather automation and the new Alder cull value.
