# Raven acknowledges a calm nearby resident (2026-10-08)

When grounded or perched, the Raven now gives one brief, restrained glance to an
awake conscious resident within 5 m who is moving no faster than 1.8 m/s and
within 2.5 m vertically. The procedural Raven turns its head; the preferred
imported Crow uses a subtle body-relative turn because it has no separate head
pivot. Attention fades over 1.8 seconds, does not move the Raven or the
resident, and does not speak, write memory, or request a model turn. The brief
gaze tracks the resident while they remain calm and nearby, and
fades early if they leave, become active, sleep, or the raven settles into
flight/rest. Each awake resident is acknowledged at most once while the nearby
group remains present, including when several residents are close; the group
rearms after everyone moves beyond the 7 m forget radius. Flight and other
attention responses take precedence. A per-group weak set prevents two close
residents from causing the raven to alternate repeatedly between them; when
several unacknowledged residents are eligible, it chooses the closest first.

`CaptiveSky2.Agent.RavenPerch` covers airborne suppression, fast-mover and
sleeping-resident suppression, target tracking, unchanged Raven position, early
release, fade, no immediate retrigger, and group re-arming after everyone leaves
and returns. The procedural Raven fallback also turns its head toward the
resident.

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

**Rigged Crow attention follow-up (2026-10-08):** the first real-RHI visual
capture attempt exposed that the preferred imported Crow has no
`RavenHeadPivot`; its setup returns before constructing the procedural head.
The previous `UpdateHeadAnimation` therefore returned immediately on the
preferred art, making every environmental/resident attention cue invisible.
The old head-turn assertions were also guarded by `if (HeadPivot)` and thus
skipped the imported-Crow case. The rigged path now applies a restrained,
reversible body-relative turn (clamped to 12 degrees yaw and 5 degrees pitch)
for listening-stone, minnow, and resident attention while settled, and restores
the authored orientation on fade, flight, or rest. Raven's capsule/location and
the resident remain unchanged. `CaptiveSky2.Agent.RavenPerch` now exercises the
preferred rigged Crow path: calm-resident selection, a small visible turn, no
Raven movement, early release, target release, and return to rest orientation.
The isolated editor target rebuilt with UHT and linked successfully; the
automation test passed under UE 5.8.3 `-NullRHI`, with agent thinking disabled,
zero model requests, a 60-second play cap, and a scratch-only data root. The
visual capture still needs a fresh run now that the rigged path is handled.

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

**Editor reachability and attention framing (2026-10-08):** a fresh Unreal MCP
`CaptureEditorImage` succeeds. The Island editor is open and rendering; a
separate Profile Data Visualizer window is simply covering much of it. The
editor still shows `1 Unsaved`, so no restart/reboot or UI dismissal was needed
to diagnose reachability. A real-RHI `CaptiveSky2.Visual.RavenWingMotion`
capture completed successfully under UE 5.8.3, agent thinking disabled, zero
model requests, and a 60-second cap. Four elevated camera candidates confirmed
that visibility-channel traces do not predict rendered occlusion in this
fixture. The selected reverse-X angle and an Aster-facing transient actor pose
now show both Aster and the Raven clearly, with the bird looking toward Aster;
the latest image is
[`03_ResidentAttention.png`](../../Saved/CompileScratch/Codex_RavenAttentionValidation_20261008/Project/Saved/Viewpoints/RavenWingMotion_/20261008_053557/03_ResidentAttention.png).
This is readable behavior evidence, not a beauty shot: the editor-world ground
and blockout props remain visible, and a real Game/PIE scene review is still
needed before judging the moment in its natural habitat. The isolated target
rebuilt successfully after the fixture adjustment; the final real-RHI test
reported `Result={Success}`.

**Imported Crow flight-pose capture (2026-10-08):** review of the same visual
test found its two flight screenshots were identical: the automation manually
advanced the AI controller but did not advance the skeletal component's
single-node animation clock. The fixture now samples two separated times in the
selected flight sequence, refreshes the skeleton, and asserts that at least one
bone transform changes before saving the second frame. The isolated UE 5.8.3
target rebuilt successfully and the real-RHI `CaptiveSky2.Visual.RavenWingMotion`
test again reported `Result={Success}` with thinking disabled, zero model
requests, and a 60-second cap. The rendered [first stroke](../../Saved/CompileScratch/Codex_RavenAttentionValidation_20261008/Project/Saved/Viewpoints/RavenWingMotion_/20261008_054450/05_FlightStrokeA.png)
and [second stroke](../../Saved/CompileScratch/Codex_RavenAttentionValidation_20261008/Project/Saved/Viewpoints/RavenWingMotion_/20261008_054450/06_FlightStrokeB.png)
now show distinct wing poses. This proves the imported clip renders at two
sampled phases; a natural, ticking Game/PIE flight remains the in-world visual
check.
