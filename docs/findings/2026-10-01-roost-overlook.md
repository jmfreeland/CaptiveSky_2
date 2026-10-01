# West-roost rendered checkpoint (2026-10-01)

The UE 5.8.3 `CaptiveSky2.Visual.Viewpoints` automation completed successfully against the isolated `AgentMovementAutomationProjectWithContent` copy. It rendered `04_WindArchOverlook` at 17:00 and reported the saved Island coordinates for RavenPerch, Roost_West, and Roost_East. The capture ran with agent thinking disabled, an isolated data root, a 120-second real-time ceiling, a one-request ceiling, and `ViewpointNoWorldState`; it started no play session and wrote no persistent world state.

The golden-hour frame reads as a sheltered, warm overlook, but the large foreground rock and WindArch structure dominate the composition and partly obscure the West-roost perch. The raven is absent by design: this editor-world capture proves rendering and camera setup only, not the bird's rendered body, arrival, or wind-driven roost choice. The next visual checkpoint should find a clearer angle that keeps the arch as a frame while exposing the perch; a later bounded PIE capture should assess the real raven and nearby wildlife without enabling background thought or model calls.

Capture: `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-10-01_040746_h17.0/04_WindArchOverlook.png`

Log: `Saved/Logs/Codex_RoostOverlook_20261001_retry.log` (test result `Success`; the log also contains blocked outbound service connection warnings unrelated to the test).

## Bounded Game-world follow-up

The first rendered spectator launch was malformed by PowerShell native-argument splitting: paths with spaces became separate arguments, and UE asserted while trying to create a shader transfer file whose path contained a literal quote. The corrected `Start-Process` quoting kept each `-key=value` path intact. This was a launch-harness failure, not a CaptiveSky gameplay crash.

The corrected UE 5.8.3 Game-world run exited normally at the configured 120.1-second safety limit, with background resident thinking disabled, a one-request ceiling, zero requests made, and startup state confined to `Saved/Playtests/Codex_WorldEcologyVisual_20261001`. Five 1600x900 spectator frames were captured through Tideglass; the run did not reach the later WindArch roost shot before the cap. The in-game Tideglass frame shows the shallow pool as a nearly featureless white disc, making the water and any subtle wildlife response difficult to distinguish. This is a concrete material/readability issue; do not change the user-owned pool asset until its assignment is inspected and a reversible prototype can be compared.

The run emitted nine `LogPython: Error` tracebacks from UE Experimental Toolsets importing unavailable editor-only Python symbols (`ToolsetDefinition`, `AgentSkill`, and related APIs). They are the same engine-plugin startup issue recorded in the long-run findings, not CaptiveSky gameplay exceptions; the Game world reached its cap and shut down normally.

The spectator screenshot writer originally used fixed filenames directly in `Saved/Screenshots/Spectator`; this capture replaced the previous `003_Inn_Common_Room.png` because that title matched. The other four new shot titles did not collide, and no duplicate copy of the replaced frame was found under `Saved/`. An opt-in `-ScreenshotDirectory` / `-SpectatorScreenshotDir` override now preserves the historical default while isolating each requested capture. Relative overrides are rooted at `Saved/` (for example, use `Screenshots/Spectator/ReturnCheck`, not `Saved/Screenshots/...`).

Validation initially appeared to stall because the sandbox denied UBT access to its per-user log/trace folder; after build access was approved, the UE 5.8.3 editor target compiled successfully and `CaptiveSky2.Agent.Spectator` passed. Its path-resolution test exposed that `FPaths::ProjectSavedDir()` may itself be relative in this launch context, so the resolver now canonicalizes the Saved root and returns a full path. A 60-second Game run with thinking disabled and a one-request hard cap made zero requests, exited normally at 60.3 seconds, and queued three 1600x900 frames in the intended new folder without overwriting the historical directory. The clear `001_Shore_Approach.png` capture also confirms the frame is no longer covered by shader-preparation text. The `003_Inn_Common_Room.png` view is still poorly composed; camera framing there is a separate visual follow-up.

The first safety run passed `Saved/Screenshots/...` as a relative override, so by contract it landed under `Saved/Saved/Screenshots/...`; this was isolated and did not overwrite any earlier frame. Examples and README now show the correct relative form. The Game run still does not prove a raven arrival or wind-driven choice.

Game frame: `Saved/Screenshots/Spectator/005_Tideglass.png`

Isolation frame: `Saved/Screenshots/Spectator/Codex_ScreenshotIsolation_20261001_retry/001_Shore_Approach.png`

Game log: `Saved/Logs/Codex_WorldEcologyVisual_20261001_corrected.log`

Isolation log: `Saved/Logs/Codex_ScreenshotIsolation_20261001_retry.log`

Automation log: `Saved/Logs/Codex_SpectatorAutomation_20261001_retry.log`

## Raven flight-wander curiosity

Raven flight-wander now has a 40% chance to prefer one of twelve sampled cruise destinations that makes the most progress toward a nearby, currently visible actor tagged `IslandLandmark`; otherwise it retains the existing random cruise choice. It reuses the grounded residents' landmark-progress scorer and logs the selected landmark when the bias produces forward progress. This is a soft attraction, not a waypoint obligation or an LLM action.

UE 5.8.3 editor build succeeded and `CaptiveSky2.Agent.RavenPerch` passed, including deterministic checks for curiosity selection and both random-fallback cases. Earlier provider-free Game probes completed both ordinary Raven movement branches in separate isolated launches: a 138 cm ground hop and a flight-wander ending at the reported destination; both made zero model requests. Those launches did not happen to log a curiosity selection, so the movement harness was extended with an optional start override and a forced-curiosity diagnostic branch. From Aster's logged exploration start `(-100400, 100960, 2749)`, the live Raven saw two visible nearby landmarks and selected a cruise target gaining 1,073 cm toward `WindArch`. He physically completed the flight after 1,220 cm in 4 simulated seconds; the Game run made zero model requests and exited normally. The diagnostic bypasses only the ordinary 40% random gate; it uses Raven's production sightline scan, candidate scoring, and flight movement. It is not evidence that an LLM autonomously requested wander or that a camera-framed player would necessarily see the marker.

A subsequent Game probe used the exact same start and ordinary `Wander` action, with no curiosity override. This time the normal random gate fired: Raven saw two visible nearby landmarks, selected WindArch (+1,220 cm progress), and reached the flight destination after 1,660 cm in 5 simulated seconds. It made zero model requests and exited normally. This confirms the production soft-curiosity path can be selected naturally and carry Raven to its destination; it does not estimate the 40% rate statistically, prove an LLM independently chose wander, or guarantee that every player camera would frame WindArch.

The final UE 5.8.3 editor build succeeded after the diagnostic change, and `CaptiveSky2.Agent.RavenPerch` passed again. The movement probe now supports selecting a mover by actor tag or resident ID, optional wander start coordinates, and Raven's custom locomotion completion instead of relying only on nav path-following status.

Automation logs: `Saved/Logs/RavenLandmarkCuriosityTest.log`, `Saved/Logs/RavenCuriosityAutomation_20261001.log`

Forced-curiosity game probe: `Saved/Logs/MovementProbe_Agent_Raven_01_Wander_Curiosity.log`

Natural curiosity game probe: `Saved/Logs/MovementProbe_Agent_Raven_01_Wander.log`
