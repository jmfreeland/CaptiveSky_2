# Shore-crab response to Tideglass ripples (2026-10-07)

Shore crabs now notice nearby natural wind- and rain-made Tideglass surface
ripples. Within 275 cm they briefly scurry away from the ripple source, then
resume their existing local path. The check is sampled every 0.35 seconds and
has an eight-second response cooldown. It ignores untagged visitor ripples,
does not interrupt an active scurry, and is disabled while the crab is sheltered.

This is a local, reversible ecology cue: it changes no habitat, ownership,
resident memory, saved world state, or model-request budget. It links an existing
weather effect to an existing shoreline resident without adding another actor
or per-frame world scan.

UE 5.8.3 validation: the `CaptiveSky_2Editor` target built and linked
successfully (11 actions). The focused `CaptiveSky2.Agent.NightEcology`
automation passed with `-NullRHI`, Python and agent thinking disabled, an
isolated data root, and a 45-second play-session cap. It covers untagged-ripple
filtering, wind and rain ripple responses, the 275 cm radius, movement away
from the source, cooldown suppression, and the regular Tick-driven response
path. Log: `Saved/Logs/Codex_CrabRipple_20261007.log`.

This is deterministic behavior evidence; no live Game/PIE view was available
to judge whether the scurry reads clearly at the shoreline. The response is
intentionally subtle and transient, so verify it with a close Tideglass view
when the editor or an interactive game session is available.
