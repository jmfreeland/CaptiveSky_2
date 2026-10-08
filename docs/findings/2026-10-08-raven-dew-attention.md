# Raven notices morning dew (2026-10-08)

When the optional dew material is present and the local glints are strong enough,
a grounded or perched raven can give the nearest clear, nearby glint one brief
look. The cue is limited to 8.5 m, requires an unobstructed visibility trace, and
lasts about 1.8 seconds. Flight, sleep, distant or occluded glints, a missing
material, and low dew produce no response. The raven stays in place, makes no
memory or persistent-world change, and spends no model request. The same dew
scatter cannot repeatedly retrigger the look; it rearms after the glints fade.

The focused `CaptiveSky2.Agent.RavenPerch` test passed in the UE 5.8.3 isolated
scratch editor with `-NullRHI`, agent thinking disabled, model requests capped at
zero, and a 60-second realtime cap. It checks nearby target selection, flight
filtering, no body movement, head response, fade-out, one-shot behavior, and
rearming after the dew fades. The updated `CaptiveSky_2Editor` scratch target
also built successfully. Test log:
[`Codex_RavenDewAttention_Final_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenDewAttention_Final_20261008.log).

This is behavioral automation, not a rendered dawn review or performance
measurement. The local dew material is optional and ignored by Git; if it is
missing, both the glints and the raven's cue remain absent. No PIE session was
started, and the open editor with its two unsaved Rhododendron material items
was left untouched. Next: inspect the settled raven and dew together in a
matched dawn Game/PIE view after the editor's unsaved material work is safe.
