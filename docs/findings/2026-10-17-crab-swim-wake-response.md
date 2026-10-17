# Shore crabs retreat from swimmer wakes

## Change

Shore crabs now treat a nearby `PlayerSwimImpact` surface cue as a brief
disturbance. They scurry away for the existing short response interval, then
return to their local routine; the shared eight-second cue cooldown prevents
one lingering wake from repeatedly startling them. Minnow splashes still draw
the crab toward the water, natural weather cues retain their existing response,
and unclassified ripples remain ignored.

This extends the swimmer interaction across the pond edge: nearby minnows
widen their school, dragonflies investigate, and shore crabs briefly retreat.
The response is local, temporary, and non-LLM; it does not alter homes or
persist state.

## Regression coverage

`CaptiveSky2.Agent.NightEcology` now checks that a tagged nearby player wake
causes an away-from-wake scurry, that the response uses its bounded cooldown,
and that the same cue cannot retrigger immediately. Existing minnow, weather,
and untyped-ripple checks remain in place.

## Validation status

The isolated UE 5.8.3 `CaptiveSky_2Editor` Development target built and linked
successfully from `Saved/CompileScratch/Codex_VisitorResonance_20261017`
(UBT exit code 0). Automation and live visual behavior were not verified in
this turn: the editor window is not exposed to the current desktop connection,
and the current scratch project-open/test path previously failed before test
startup with the F405610C signature. See
[`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md).
