# Tidepool periwinkles notice scurrying crabs

The Tideglass already has independent crab and periwinkle routines: crabs
scurry from nearby visitors and natural water impacts; periwinkles tuck for
the ebb, heavy rain, Raven approaches, and close observation. The two species
did not otherwise react to each other.

A periwinkle now briefly tucks when a tidepool crab is actively scurrying
within 175 cm and 100 cm vertically. A calmly foraging or sheltered crab does
not trigger it. Each periwinkle responds once per scurry; the cue rearms when
the crab settles or retreats beyond 500 cm. Existing Raven and player cues are
checked first, and the ordinary observation cooldown still limits how often a
snail can tuck. It adds no crab movement, persistent state, resident schedule,
asset, or model request. The neighborhood check reuses the existing 0.4-second
disturbance cadence and scans the two Tideglass crabs.

The new focused automation covers an in-range scurry, the radius boundary,
calm-crab suppression, range and height rejection, and malformed distance.
The UE 5.8 `CaptiveSky_2Editor` target compiled and linked the new behavior and
test in the isolated `Codex_BasinRipple_20261018` scratch project (9 actions,
21.15 seconds). The automation was not launched: project-opening tests still
crash before test startup with the known Core null-access hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The live editor is also still
unresponsive to activation and reports one unsaved change, so the cue is not yet
runtime- or visually verified. The editor and map were left untouched.
