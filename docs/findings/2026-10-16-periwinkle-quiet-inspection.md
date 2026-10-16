# Quiet periwinkle inspection — 2026-10-16

Tidepool periwinkles are now available through the shared visitor/resident
inspection resolver, alongside the other small Tideglass wildlife. They remain
wild-life targets only: `IsMovementTargetAllowed` still rejects all
`IslandLife`, so agents cannot path to or chase a snail. A quiet inspection
prompts a 1.6-second shell-tuck cue and a 4.2-second response cooldown. The cue
temporarily combines with the existing low-tide posture; it changes no
position, population, ownership, memory, or saved world state. The visitor
response explicitly describes observation rather than touching or collecting
the animal.

`CaptiveSky2.Agent.NightEcology` now has regression source for target resolution,
movement exclusion, visitor visibility/range, the transient shell tuck,
cooldown wording, recovery to the high-tide posture, and unchanged
nonpersistent status. Its fixture holds the Island clock at high water for the
visual-posture assertions, then restores the prior time.

The UE 5.8.3 editor target compiled and linked successfully in the isolated
scratch project after the final test-fixture edit (4 actions; only
`IslandEcologyTests.cpp` recompiled). The automation test and in-world
interaction were not run: the project-opening startup crash remains
unsymbolized, and no Unreal Editor/Game process was launched. Once the matching
editor symbols are installed and that crash is diagnosed, run
`CaptiveSky2.Agent.NightEcology`, then inspect this tiny response at ordinary
visitor distance to confirm the prompt is useful and the tuck remains subtle.
