# Tideglass periwinkle life — 2026-10-16

Three small periwinkles now occupy ground-traced positions around the Tideglass
shoreline. Each uses a compact primitive fallback body with a procedural shell
growth spiral and one of three muted natural shell colors. Their components are
non-colliding, do not affect navigation, and use a slow antenna sway. The
bounded population persists through the Island's day/night cycle and is cleaned
up with the rest of transient Tideglass ecology; no map, Content asset, or
persistent world state is changed.

**Close-visitor response (2026-10-16):** periwinkles now turn their eye stalks
toward a player pawn in front of them and within 220 cm, easing the glance in
and settling back more slowly while leaving the shell and ground-traced home
in place. The response is capped at 35 degrees, runs on the actor's existing
0.2-second tick, and does nothing when no player pawn exists or the player is
behind. Deterministic
automation assertions cover forward, side, behind, and out-of-range attention.
The source and assertions are compiled and linked in the isolated scratch
build below; they still need automated execution and visual review.

## Verification

- The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully in the
  isolated `Saved/CompileScratch/Claude_Props` project after the new actor,
  weather integration, and `IslandEcologyTests.cpp` compiled.
- The test adds checks for the three-member cap, shoreline placement, shell
  geometry, distinct palettes, no-collision behavior, idempotent refresh, and
  persistence through day and night.
- The attempted NullRHI `CaptiveSky2.Agent.NightEcology` run exited with code
  zero but did not create its requested log or a test result. Treat it as an
  unsuccessful startup, not a passing test. The retained crash context was not
  updated by this attempt, so its cause remains unknown; see
  [`Unreal startup crash triage`](2026-10-13-unreal-startup-triage.md).
- A read-only probe of the local editor MCP configured in `.mcp.json`
  (`http://127.0.0.1:8000/mcp`) also failed with an `HttpRequestException`, and
  no listening TCP entry for port 8000 was visible. This session therefore
  cannot use the editor MCP as an alternate automation runner. This is separate
  evidence from the headless startup failure and does not identify its cause.
- No real-RHI capture exists yet. The procedural silhouette, palette, ground
  placement, visitor gaze, and close-range readability remain unreviewed
  in-game.

**Follow-up build check (2026-10-16):** with the main editor open, UBT compiled
`IslandTidepoolPeriwinkle.cpp` and `IslandEcologyTests.cpp` successfully, but
could not replace the main `Binaries/Win64/UnrealEditor-CaptiveSky_2.dll`
because that editor held it open. The documented `Saved/CompileScratch/Claude_Props`
project has its own Binaries and a `Source` junction to this working tree; its
`CaptiveSky_2Editor` target then compiled and linked successfully (7 actions),
including both changed units. This verifies compilation and linking of the
current source without touching the main editor's loaded DLL.

The editor's Live Coding tool returned `NoChanges`, so it did not patch the
main session. The MCP endpoint was live on `127.0.0.1:8000`, and its
`CaptiveSky2.Agent.NightEcology` test passed (1 passed, 0 failed), but that
result came from the already-loaded main editor module and does not verify the
new gaze assertions. No editor process was stopped and no gameplay or
model-request session was started. The editor did perform its scheduled
autosave of `/Game/Maps/Island` to `Saved/Autosaves/Game/Maps/Island_Auto1.umap`;
the authored map package was not explicitly saved by this session.

**Latest source validation (2026-10-16):** after adding a slower gaze-release
curve and its assertion, the isolated scratch target rebuilt successfully
(7 actions; `IslandTidepoolPeriwinkle.cpp`, `IslandWeather.cpp`, and
`IslandEcologyTests.cpp` compiled, then the editor DLL linked). A subsequent
Live Coding request to the main editor did not return; its log ends at
`Starting Live Coding compile`. PID 22688 remains alive but reports
`Responding=False` and holds the main project DLL. Its TCP listener is bound,
but a fresh MCP initialize request times out, so the endpoint is not usable.
No fresh crash artifact was created. The editor was not terminated because it
may contain unsaved work.
The new gaze test has not run against this rebuilt code, and no visual review
has been made. Next: once the editor safely recovers or is closed, rebuild the
main target, run `CaptiveSky2.Agent.NightEcology` against that build, then
inspect a short shoreline view.

**Bounded scratch automation attempt (2026-10-16):** after the latest scratch
build succeeded, `UnrealEditor-Cmd.exe` was invoked on
`Saved/CompileScratch/Claude_Props/CaptiveSky_2.uproject` with `-NullRHI`,
`-Unattended`, `-NoSound`, `-DisablePython`,
`-CaptiveSkyMaxModelRequests=0`, and only the
`CaptiveSky2.Agent.NightEcology` automation test. The process exited with code
0 but did not create the requested log, and no new crash artifact or retained
process appeared. This is an unsuccessful startup, not a passing test; do not
repeat the same launch unchanged. The main editor (PID 22688) remains alive but
unresponsive and holds the main project DLL. Its port-8000 listener remains
bound, but requests time out. No process was terminated.

## Next step

After the editor recovers or is safely closed, build the main target and run
`CaptiveSky2.Agent.NightEcology` with resident thinking disabled and a zero
model-request cap. Then make one short, fixed Tideglass shoreline capture to
review scale, gaze, and material response before deciding whether the fallback
is good enough or should be replaced with a proper snail mesh. Do not repeat
the scratch `UnrealEditor-Cmd` launch unchanged; it exited without a log.
