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

The first live close-up showed the shell as pale default material. Follow-up
inspection confirmed that the engine's parameterized shape-material instance
exposes `Color`; the runtime-spawned crab now resolves that material in
`BeginPlay` as a fallback, applies a darker rust-and-sand palette, and uses a
flatter carapace profile. The UE 5.8.3 editor target rebuilt successfully, and a
matched 1600x900 noon Game capture visibly confirmed the warm shell and lighter
appendages. That capture used an isolated world-state root, disabled agent
thinking/Python, a 25-second real-time cap, and ended with zero model requests.
Screenshot: [close Tideglass crab view](../../Saved/Playtests/Codex_CrabAppearance_20261007/VisualScreenshotsFinal/001_Crab__Close.png).

This visual spot check confirms the material and silhouette, not the transient
ripple-triggered scurry. The close camera also shows that dense foreground
ground cover obscures parts of the legs; the crab is still procedural primitive
geometry rather than a final imported animal asset. Revisit those two limits
when a closer shoreline composition or suitable crab mesh is available.
