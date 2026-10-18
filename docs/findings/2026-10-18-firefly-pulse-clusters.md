# Firefly pulse clusters

Nearby fireflies now develop a restrained shared rhythm over time, rather than
flashing with wholly independent timing. Each insect checks its local group at
most once every 0.4 seconds, within a 4.6 m horizontal radius and 1.8 m height
difference. A small phase nudge and pulse-rate adjustment draw it toward the
nearby mean; the adjustment is bounded and uses a separate glow phase, so flight
paths and the shared random sequence are unchanged. Coupling eases in at dusk,
is absent by day and during strong rain, and pauses during the firefly's brief
response to a visitor or Listening Stones chime. It adds no actors, lights,
model requests, or persistent state.

The new `CaptiveSky2.Agent.FireflyPulseClusters` regression covers calm-night
coupling, gradual dusk response, daylight/rain/distance isolation, wrap-safe
bounded phase nudges, gradual pair convergence, and independence of the flight
phase. The current UE 5.8.3 main-project build compiled both
`IslandFirefly.cpp` and `IslandFireflyTests.cpp`. Linking did not complete:
UnrealBuildTool reported that `UnrealEditor.exe` held
`Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` open. The editor was not stopped.

Runtime behavior remains unverified. A bounded NullRHI automation launch in
the isolated content-light project crashed before creating a project log or
starting the test, at `SecondsSinceStart=0` with the existing null-access
signature (`F405610C114D120D4B6F5ABA901EC469AC9FAA26`). No assertion result is
claimed. Once the project startup issue is meaningfully resolved and the editor
releases the DLL, relink and run this focused test with model requests disabled,
then inspect the effect in a real-RHI night view before judging its legibility
or tuning the coupling.
