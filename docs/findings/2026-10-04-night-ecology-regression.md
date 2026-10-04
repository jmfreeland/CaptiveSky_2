# Night ecology automation follow-up (2026-10-04)

The connected UE 5.8.3 editor was idle (Island loaded; PIE was not running) when a focused
regression set ran. `IslandNest`, `TidepoolMinnows`, and `PersonalityConsolidation` passed.
`NightEcology` failed twice at the saved-Island assertion that weather creates a transient
dynamic cloud material. The Island's `VolumetricCloud_0` is an `AVolumetricCloud`, and its
authored `m_SimpleVolumetricCloud_Inst` exposes the expected coverage, density, and storm
parameters, so the failure is not explained by an absent cloud actor or material.

The test restores the cloud material and clears `CloudComponent`/`WeatherCloudMaterial` after
probing, but it does not restore `NextCloudDiscoveryTime`. Runtime cloud discovery is throttled
against world time, which can remain nearly static in the editor world. The test now saves that
private transient value, clears it before its explicit material-creation probe, and restores it
with the rest of its test state. This is a test-isolation hypothesis, not yet a proven fix.

The modified `IslandEcologyTests.cpp` compiled as a scratch-only object with the project's
preserved UE 5.8 / MSVC 14.44 response file; the compiler SARIF had no diagnostics. The first full
isolated UBT rebuild emitted no build actions or log output for about 2.5 minutes, so only that
launcher was cancelled. To finish the verification without touching the running main editor, the
changed test object was relinked into the scratch editor module using its preserved link response
file. The scratch project had the same `IslandWeather.cpp`, `TidepoolMinnows.cpp`,
`TideglassDragonfly.cpp`, and unmodified test source as the current checkout; its saved Island map
was loaded with isolated DDC settings. `CaptiveSky2.Agent.NightEcology` then passed, logging
`Weather linked cloud material m_SimpleVolumetricCloud_Inst` and exiting with code 0. This confirms
the stale discovery throttle was the test-isolation issue. The original dynamic-material and
weather assertions remain intact.

In the connected main editor, the surrounding `IslandNest`, `TidepoolMinnows`, and
`PersonalityConsolidation` checks also passed; only the pre-fix `NightEcology` run failed there.
No main editor state, source runtime code, or Content/map asset was changed by verification.
