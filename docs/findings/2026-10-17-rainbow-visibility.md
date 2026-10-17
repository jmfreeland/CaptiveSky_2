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

Validation: UE 5.8.3 `CaptiveSky_2Editor` built successfully in the existing
`Saved/CompileScratch/Claude_Props` project (7 actions, including
`IslandRainbow.cpp` and `IslandRainbowTests.cpp`). The focused automation test
compiled but was not run: opening this project for NullRHI automation has
repeatedly crashed before tests begin with the documented Core null-access
startup failure (`F405610C114D120D4B6F5ABA901EC469AC9FAA26`). No Content assets
or level data were changed. Once that startup issue is resolved, run
`CaptiveSky2.Agent.IslandRainbow` and verify both a renderable material and the
missing-material path; in-world visual review remains pending.
