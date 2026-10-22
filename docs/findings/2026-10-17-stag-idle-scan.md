# Stag ambient idle scan — 2026-10-17

The woodland stag now occasionally pauses its ordinary grazing loop to play the
existing one-shot look-around animation, then returns to grazing. The pause is
scheduled a few seconds into an idle interval and is rate-limited before another
can occur. It neither moves the stag, starts a resident request, nor changes
persistent world state. Existing resident, visitor, fox, raven, thunder, chime,
and rest responses retain priority over this cosmetic idle cue.

The focused `CaptiveSky2.Agent.WoodlandDeer` automation was extended to check
that the ambient look uses the existing animation, leaves position and startled
state unchanged, returns to grazing, and schedules a substantial cooldown.

## Validation status

The ordinary main-tree UE 5.8.3 editor-target build was correctly refused by
UBT because it detected the user's active Live Coding mutex. To keep the
editor's loaded project DLL untouched, a dedicated scratch project was built
with its own `Binaries`, linked to the current `Source`, `Content`, and
`Plugins`, and built with `-NoHotReloadFromIDE -MaxParallelActions=3`. The full
173-action editor target succeeded, including both `IslandForestStag.cpp` and
`IslandForestStagTests.cpp` compilation and the scratch module link.

At the time of the scratch-build attempt, the focused
`CaptiveSky2.Agent.WoodlandDeer` run was **not verified**. The
headless process exited 0 immediately without creating the requested Unreal
log, automation transcript, stdout, or stderr, so there is no evidence it
reached engine startup or ran the test. No PIE or visual capture was run. Do
not treat the immediate exit as a pass; retry the automation only after the
known Unreal commandlet startup issue is diagnosed or launch conditions
meaningfully change. No runtime appearance or pacing claim is made yet.

## Live editor automation recheck (2026-10-22)

The current UE editor's focused `CaptiveSky2.Agent.WoodlandDeer` automation
test passed in 0.056 s with no errors or warnings. Its assertions cover the
existing animation assets and skeleton compatibility, the temporary ambient
look-around and return to grazing, bounded movement, and the animal's other
perception responses. This resolves the missing automation evidence from the
headless attempt, but it is not a live Island PIE or visual confirmation that
visitors see the deer perform the cue.

Scratch build output: `Saved/CompileScratch/Codex_StagIdleScan_20261017`.
The failed main-tree build transcript is `%LOCALAPPDATA%\\UnrealBuildTool\\Log.txt`
on the machine.
