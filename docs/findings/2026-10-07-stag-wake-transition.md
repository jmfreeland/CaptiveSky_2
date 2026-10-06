# Woodland stag wake transition (2026-10-07)

When the day/night routine wakes the woodland stag, it now plays the imported
`ANIM_DeerStag_SleepToGoBackUp` one-shot before returning to its ordinary graze
loop. The animation has no root motion; the actor stays in its bounded home
patch. A startling nearby thunderclap can still interrupt the transition and
move the stag away from the sound. If the pack animation is unavailable, waking
falls back to grazing as before.

Validation on UE 5.8.3: the `CaptiveSky_2Editor` target built successfully, and
`CaptiveSky2.Agent.WoodlandDeer` passed with `-NullRHI`, Python and agent
thinking disabled, zero model requests, isolated `-DataRoot`, and a 45-second
real-time cap. The test covers animation/skeleton/root-motion compatibility,
one-shot transition timing, return to grazing, and the existing bounded thunder
response. Log: [`Codex_StagWake_20261007.log`](../../Saved/Logs/Codex_StagWake_20261007.log).

The current computer-use connection did not expose an Unreal window, so the
animation's appearance under Island lighting has not yet been visually checked.
