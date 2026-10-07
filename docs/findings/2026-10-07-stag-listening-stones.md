# Woodland stag notices the Listening Stones (2026-10-07)

The woodland stag now notices a live Listening Stones chime when it is within
the chime's existing audible radius. A grazing stag plays the imported
`ANIM_DeerStag_IdleLookAround` one-shot, stays in place, and then resumes its
ordinary routine. The response has a 10-second cooldown, cannot restart while
already playing, and is skipped while the stag is resting, waking, moving, or
startled. It changes no saved world state, resident memory, ownership, or model
request budget.

Validation on UE 5.8.3: the `CaptiveSky_2Editor` target built successfully, and
the focused `CaptiveSky2.Agent.WoodlandDeer` automation passed. It verifies the
animation asset and skeleton, audible-radius gating, one-shot selection,
unchanged position, cooldown, and return to grazing. The run used `-NullRHI`,
disabled Python and agent thinking, allowed zero model requests, and had a
45-second real-time cap. Log:
[`Codex_StagListeningStones_20261007.log`](../../Saved/Logs/Codex_StagListeningStones_20261007.log).

The automation does not establish that the look-around reads naturally under
Island lighting or that the stag and Listening Stones are framed together in
ordinary play. That visual check remains pending an accessible UE viewport.
