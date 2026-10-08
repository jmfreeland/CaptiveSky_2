# Raven notices a nearby shore-crab scurry (2026-10-08)

When a Tideglass crab is already scurrying from a disturbance, a grounded or
perched raven within 7 m and 3.2 m vertically can briefly turn toward it. The
glance lasts about 1.5 seconds and fades back into the existing idle scan. It
does not move the raven, pursue or startle the crab, write memory or world
state, or request a model turn. A weak actor reference prevents the same
scurry from repeatedly restarting the glance; a later scurry can be noticed
after the first one ends. The check uses the raven controller's existing
quarter-second attention cadence rather than adding a tick or world scan.

This completes a small local ecology chain: the crab's existing response to a
quiet raven/flyby produces a visible scurry, and the raven can acknowledge that
movement without escalating into a chase. Other states and the existing
listening-stone, minnow, and resident-attention cues keep their own priority.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` scratch target built successfully in an isolated
  project with its own `Binaries` and `Intermediate` directories (156 actions,
  259.47 seconds). `-NoHotReloadFromIDE` kept the user's open editor untouched.
- `CaptiveSky2.Agent.RavenPerch` passed in a headless NullRHI run (exit code 0).
  The provider-free regression covers grounded attention, unchanged Raven
  position, a fading glance, no retrigger during the same scurry, and a fresh
  glance after a later scurry. Python and resident thinking were disabled,
  model requests were capped at zero, and test world data was isolated.
- The headless editor logged a successful test and exit request but remained
  resident after 30 seconds without further log output; only that scratch
  process was stopped. The user editor remained open and untouched.

Log: [`Codex_RavenCrabAttention_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenCrabAttention_20261008.log).

This verifies the bounded behavior path, not whether the head turn is readable
at ordinary play distance or whether the crab/raven timing feels natural in the
saved Island. Recheck that presentation in a short Game/PIE view before tuning
the cue radius or duration.
