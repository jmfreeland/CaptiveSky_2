# Woodland stag notices a low raven flyby (2026-10-07)

The wild stag now notices a raven only while its controller reports active
flight, within 7 m horizontally and 1.8–8 m above the ground. A close pass
briefly sends a grazing stag away from the wing shadow using its existing
grounded, 7 m-bounded startle movement; it then returns to grazing. The response
has a 12-second cooldown and does not affect a resting, waking, or already
moving stag. It adds no resident decision, model request, persistent memory, or
predator/companion relationship.

The UE 5.8.3 `CaptiveSky_2Editor` target built successfully. Focused
`CaptiveSky2.Agent.WoodlandDeer` automation passed with agent thinking disabled
and `-NullRHI`, covering direction, range, altitude, raven flight state,
resting-state protection, bounded movement, cooldown, and the periodic sensing
path. Log: [`Codex_StagRavenFlybyRetry_20261007.log`](../../Saved/Logs/Codex_StagRavenFlybyRetry_20261007.log).
No gameplay session or resident model requests were involved.

The test does not establish whether the response reads clearly or whether the
frequency feels natural in a rendered play session; that still needs visual
review in the interactive editor.
