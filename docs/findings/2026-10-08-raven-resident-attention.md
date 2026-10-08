# Raven acknowledges a calm nearby resident (2026-10-08)

When grounded or perched, the Raven now gives one brief, restrained head glance
to an awake conscious resident within 5 m who is moving no faster than 1.8 m/s
and within 2.5 m vertically. Attention fades over 1.8 seconds, does not move the
Raven or the resident, and does not speak, write memory, or request a model
turn. The brief gaze tracks the resident while they remain calm and nearby, and
fades early if they leave, become active, sleep, or the raven settles into
flight/rest. Each awake resident is acknowledged at most once while the nearby
group remains present, including when several residents are close; the group
rearms after everyone moves beyond the 7 m forget radius. Flight and other
attention responses take precedence. A per-group weak set prevents two close
residents from causing the raven to alternate repeatedly between them; when
several unacknowledged residents are eligible, it chooses the closest first.

`CaptiveSky2.Agent.RavenPerch` was extended to cover airborne suppression,
fast-mover and sleeping-resident suppression, the brief head turn, target
tracking, unchanged Raven position, early release, fade, no immediate
retrigger, and group re-arming after everyone leaves and returns. The initial
UE 5.8.3 UHT step passed. After the per-group weak-set, nearest-resident
selection, and moving-target/early-release assertions, both changed translation
units compiled directly with the project's cached UE 5.8.3 MSVC response files.
This does not replace a fresh UHT pass or full editor-target link. Linking could
not replace the loaded `UnrealEditor-CaptiveSky_2.dll`;
Live Coding is disabled in this editor session. The RavenPerch automation has
therefore not run against the new code, and visual readability remains
unverified.

The editor is the hidden process launched for MCP recovery, but its Island level
shows `Island*` / `1 Unsaved`. It was left open to avoid discarding or saving
unknown map state. Next: after the unsaved-level choice is resolved (or Live
Coding is enabled), rebuild/link, run `CaptiveSky2.Agent.RavenPerch`, and take a
short in-world look at the glance. Keep the cue nonverbal and non-persistent if
presentation tuning is needed.

**Editor-session follow-up (2026-10-08):** Unreal MCP can currently see the
`CaptiveSky_2 - Unreal Editor` window, and Windows reports its process as
responsive. The Editor Preferences Live Coding section reports `bEnabled=true`
and `startup=AutomaticButHidden`, but the Live Coding MCP compile tool still
reports that Live Coding is not enabled for this session; invoking the
documented `Ctrl+Alt+F11` shortcut did not change that result. No editor or PC
restart was attempted because the Island still has one unsaved level. This is
an editor-session activation issue, not evidence that the project needs a full
computer reboot. If a linked build is still needed, first resolve the unsaved
level deliberately, then reopen the editor (or test the Live Coding console
activation in a session without user data at risk).
