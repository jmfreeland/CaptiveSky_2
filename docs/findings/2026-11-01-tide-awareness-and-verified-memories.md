# Tide awareness and verified interaction memories

## Evidence and changes

In the isolated 15-minute resident session, Rat and the Innkeeper separately
asked to understand how the shoreline changes as the tide falls. Tideglass
already supplies a bounded tide-height cue, so resident perception now also
describes whether the water is flooding, ebbing, or turning at high/low water.
The trend is derived from deterministic tide samples a quarter Island-hour
apart, including across the Island-day boundary; it creates no new actors,
assets, or ticking work.

The same session exposed a memory-truth bug. At 10:19:52 UTC Raven's RainBasin
interaction was rejected as a recent repeat inspection. The isolated basin
state (`Saved/Playtests/Codex_ResidentPacing_20261101/WorldState/Island.basin.json`)
contains no leaves, but Raven's memory at that timestamp said, “After rain
filled the stone basin, I set a leaf afloat on its water.” The model memory was
written while parsing the decision, before the controller executed it.

The prompt now explicitly says the chosen action has not yet happened, and
decision parsing does not store model-proposed memories on an `interact`
response. A later decision may remember the controller's verified result. A
regression case checks that a proposed RainBasin success memory is not stored;
existing idle-reflection storage remains covered.

## Validation and remaining uncertainty

The tide-direction and resident-perception assertions, and the premature
interaction-memory regression, are in the automation suite. A prior isolated
UE 5.8.3 editor build compiled the tide-direction change. The follow-up focused
commandlet attempt crashed before engine/test startup (`SecondsSinceStart=0`,
null access violation, hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`), matching
the recurring headless bootstrap failure; no automation assertions ran. Rebuild
the isolated target with the memory guard, then run the focused test only after
that bootstrap issue is diagnosed or the startup environment meaningfully
changes. Do not count compilation as a passing runtime test.

The full-hour flood/ebb language and transition wording also need a rendered
play check. No long run was started: recent available GPU headroom was below
the project's 4 GiB gate. Canonical Island state and the open editor were not
changed.
