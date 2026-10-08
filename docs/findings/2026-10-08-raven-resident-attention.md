# Raven acknowledges a calm nearby resident (2026-10-08)

When grounded or perched, the Raven now gives one brief, restrained head glance
to an awake conscious resident within 5 m who is moving no faster than 1.8 m/s
and within 2.5 m vertically. Attention fades over 1.8 seconds, does not move the
Raven or the resident, and does not speak, write memory, or request a model
turn. Each awake resident is acknowledged at most once while the nearby group
remains present, including when several residents are close; the group rearms
after everyone moves beyond the 7 m forget radius. Flight and other attention
responses take precedence. A per-group weak set prevents two close residents
from causing the raven to alternate repeatedly between them.

`CaptiveSky2.Agent.RavenPerch` was extended to cover airborne suppression,
fast-mover and sleeping-resident suppression, the brief head turn, unchanged
Raven position, fade, no immediate retrigger, and re-arming after leaving and
returning. The initial UE 5.8.3 UHT step passed. After the per-group weak-set
change and the sleeping-/multiple-resident assertions, both changed
translation units compiled directly with the project's cached UE 5.8.3 MSVC
response files. This does not replace a fresh UHT pass or full editor-target
link. Linking could not replace the loaded `UnrealEditor-CaptiveSky_2.dll`;
Live Coding is disabled in this editor session. The RavenPerch automation has
therefore not run against the new code, and visual readability remains
unverified.

The editor is the hidden process launched for MCP recovery, but its Island level
shows `Island*` / `1 Unsaved`. It was left open to avoid discarding or saving
unknown map state. Next: after the unsaved-level choice is resolved (or Live
Coding is enabled), rebuild/link, run `CaptiveSky2.Agent.RavenPerch`, and take a
short in-world look at the glance. Keep the cue nonverbal and non-persistent if
presentation tuning is needed.
