# Rainbow visibility and resident observation

The rainbow's material is optional. If `/Game/Materials/M_IslandRainbow` cannot
load, `AIslandRainbowActor::Apply` leaves its plane hidden; the subsystem used
to retain a positive weather-derived strength anyway, so a resident could be
told that a rainbow was visible when the player could not see one.

`UIslandRainbowSubsystem::GetStrength` now reports zero unless the transient
actor exists and has successfully created its material instance. The shared
observation resolver also clamps finite strengths and suppresses non-finite
values. This keeps the resident description tied to the actual presentation
without changing the weather calculation or making the material a hard project
dependency.

Validation: the current-source UE 5.8.3 scratch target built successfully, then
`CaptiveSky2.Agent.IslandRainbow` passed under NullRHI with agent thinking
disabled, zero model requests, a 120-second cap, and the Model Context Protocol
plugin disabled. Log:
`Saved/CompileScratch/Codex_VisitorResonance_20261017/Saved/Logs/Codex_Rainbow_CurrentSource_20261017.log`.
The test covers the weather window, horizon geometry, resident observation
thresholds, missing-material suppression, and non-finite inputs. It is a pure
automation pass: it does not load the optional rainbow material or prove its
appearance. No Content assets or level data were changed; real-RHI material and
in-world visual review remain pending.
