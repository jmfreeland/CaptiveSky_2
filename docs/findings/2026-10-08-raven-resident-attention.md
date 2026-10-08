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
That initial probe did not replace a fresh UHT pass or full editor-target link:
the link could not replace the loaded `UnrealEditor-CaptiveSky_2.dll`, and Live
Coding was unavailable in that editor session.

**Isolated UE 5.8.3 validation (2026-10-08):** to validate without disturbing
the open editor, the current `Source`, `Config`, `Plugins`, and project file
were copied into a disposable scratch project with its own `Binaries` and
`Intermediate`; its `Content` path was a junction to the project's existing
assets. The `CaptiveSky_2Editor` target completed UHT and all 142 compile/link
actions successfully in 107.58 seconds, including `RavenAgentAIController.cpp`
and `RavenPerchTests.cpp`. Then `UnrealEditor-Cmd` ran
`CaptiveSky2.Agent.RavenPerch` under UE 5.8.3 with `-NullRHI`, agent thinking
disabled, a zero model-request cap, a 60-second runtime cap, and an isolated
world-data root. The test completed with `Result={Success}` and exit code 0.
This is current-source build and deterministic behavior evidence; it does not
show the cue in the Island or validate its visual readability. The user's
interactive editor and unsaved level were not saved, closed, or modified.

The editor is the hidden process launched for MCP recovery, but its Island level
shows `Island*` / `1 Unsaved`. It was left open to avoid discarding or saving
unknown map state. Next: after the unsaved-level choice is resolved, load the
validated code into the interactive editor and take a short in-world look at the
glance. Keep the cue nonverbal and non-persistent if presentation tuning is
needed.

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

The current editor's logged launch arguments also preserve the play safeguards:
`-CaptiveSkyDisableAgentThinking`, `-CaptiveSkyMaxModelRequests=0`, and
`-CaptiveSkyMaxRealtimeSeconds=600`. The launch line contains `-unattended` but
no explicit `-LiveCoding` flag. That absence is a useful relaunch diagnostic,
not yet a proven cause of Live Coding being unavailable.
