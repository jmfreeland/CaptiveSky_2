# Visitor naturalist observation (2026-11-02)

When a visitor presses Interact with no nearby landmark or animal, the controller
now cycles through available nearby tracks, visible vegetation, and local
weather observations. The plant note reuses the species naming shared with
resident perception, checks up to three nearby visible vegetation patches
within 650 cm, and explicitly leaves the plants growing and undisturbed. It is
descriptive only: it does not harvest, write memory or world state, or call a
model. Patch discovery happens only on interaction, not every frame.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` Development target built in the
isolated C: worktree. The focused `CaptiveSky2.Agent.NearbyVegetationPerception`
automation passed under NullRHI with Zen and Python disabled. It covers the
visitor-facing name and no-disturbance wording as well as existing resident
species perception; its 3,600-tree broadleaf fixture averaged 0.297 ms over 10
warmed NullRHI samples. Log: `Saved/Logs/Codex_VisitorVegetation_NullRHI_Final.log`.

This does not yet verify the player's on-screen caption in PIE or a rendered
view. The automation fixture also logs missing optional Fab and generated plant
assets because the worktree has no `Content/Maps/Island.umap`; these did not
prevent the focused test from passing. Next visual check: in a playable Island,
press E in a quiet patch with several nearby plant species and confirm the
caption cycles between available observations without affecting vegetation.

## Seasonal field-note extension

Nearby bluebells, Tideglass marsh marigolds, and yellow-flag irises now use the
existing `AIslandDayNight` calendar and `AIslandWeather` bloom curve to add a
brief “beginning to bloom” or “in bloom” cue when appropriate. If no Island
clock is present, the note falls back to the species name without guessing a
season. This remains an on-demand observation and does not affect the existing
per-instance bloom animation or write state.

UE 5.8.3 `CaptiveSky_2Editor` Development rebuilt successfully. The focused
`CaptiveSky2.Agent.NearbyVegetationPerception` NullRHI automation passed with
assertions for spring bloom and the absence of a spring-bloom claim in summer;
the 3,600-tree prompt fixture averaged 0.404 ms over 10 warmed samples. Log:
`Saved/Logs/Codex_SeasonalNaturalist_NullRHI.log`. Caption appearance in PIE
still needs a rendered, memory-gated visual check.

## Tideglass field note

When Interact has no closer visible world target, the visitor can now receive a
local tide note if within 12 m of the `TideglassPool` marker. It uses the
current `AIslandDayNight` day/hour and existing `DescribeTide` function, so the
direction and spring/neap wording stay aligned with the world's lunar tide.
The observation only runs on key press, has no memory/world-state/model side
effects, and is absent when Tideglass or its clock is not nearby. The focused
automation covers a rising-tide caption beside the pool and its absence from a
faraway visitor. To make the no-target observation discoverable in clear,
dry weather, the outdoor hint now offers `E: notice your surroundings`; the
existing indoor suppression remains. This uses the environment subsystem's
already-available indoor value and adds no repeated actor scan to the hint
refresh timer.

UE 5.8.3 `CaptiveSky_2Editor` Development compiled and linked successfully,
including the controller and focused test. The NullRHI run was attempted once
and exited before test discovery with the known zero-second
`EXCEPTION_ACCESS_VIOLATION`, crash hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26` (`Saved/Crashes/_0000`). Therefore
the new test is compiled but **not runtime-passed**. Do not repeat the same
startup recipe until the shared bootstrap crash is diagnosed or the launch
context materially changes. Next: restore reliable automation startup, rerun
`CaptiveSky2.Agent.VisitorTideObservation`, then verify the caption in the
Island only when the existing rendered-GPU reserve allows a safe visual check.
