# Established desire paths more clearly guide resident wandering

The existing wander planner samples each complete candidate route against the
persistent ground-trail ledger. Increase the maximum path-affinity adjustment
from 3% to 15% so a clearly worn route can meaningfully break close choices,
while leaving the existing curiosity score as the gate: a zero curiosity score
still receives no trail bonus. Untrodden routes keep their original score, and
partially worn ground scales the preference linearly.

This only affects explicit wander candidate ranking. It does not schedule a
wander, override the separate visible-landmark progress score, alter path
validity, add model requests, or change persisted trail data. The 15% ceiling
keeps path history influential without making residents mechanically follow the
same route.

The isolated UE 5.8 scratch editor target built successfully (34 actions), and
`CaptiveSky2.Agent.ResidentWanderPaths` passed with `Result={Success}` and exit
code 0. The commandlet used `-NullRHI`, disabled agent thinking, set the model
request cap to zero, and wrote only to a scratch world-data root. Its log is
`Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_TrailWander_20261029_0604.log`.

Rendered behavior and the real-play frequency of explicit wandering remain
unverified; compare bounded movement logs before deciding whether the weight
needs further tuning.
