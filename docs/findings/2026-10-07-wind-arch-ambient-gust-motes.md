# Ambient Wind Arch gust motes (2026-10-07)

The Wind Arch already shows a short three-mote effect when a resident or visitor
explicitly asks for a local gust. Natural Island weather reaches the same
landmark, but did not create any visible local response there.

The transient Game/PIE stonework actor now samples ambient local wind once per
second. At 105 cm/s or above it emits one eight-second mote pass, then waits 30
seconds before it can emit again. It skips a pass when an explicit Wind Arch
mote effect is already within 9 m. The event changes no saved actor, collision,
navigation, or world-state data and adds no model call; if there is no weather
actor, it remains inert. The interaction is a visual cue for a naturally
occurring stronger wind, not a persistent weather event or resident decision.

The UE 5.8.3 `CaptiveSky_2Editor` Development target built successfully with the
full current working set (14 UBT actions); the focused follow-up rebuild also
succeeded. `CaptiveSky2.Agent.WindArchPresentation` passed in headless Unreal
automation. Its synthetic Game world now samples `AIslandWeather` with a
deterministic high-wind offset, invokes the actual stonework response, and
asserts one transient mote actor at the arch, the 30-second cooldown, no
duplicate on a second sample, and nearby-effect suppression with a short retry
delay. It also retains the calm/strong threshold assertions and saved-map proxy
recognition. This fixture makes no model calls and does not start a Game/PIE
session.

A bounded UE 5.8.3 Game capture was then run at the Wind Arch overlook at 17:00
and 17:14 with a forced 60-second storm, thinking/Python disabled, a 60-second
real-time cap, and a one-request cap. It exited normally after 60.1 seconds
with zero model requests. The screenshots contain small vertical flecks across
the view, but the mote pass is not clearly readable as an intentional airflow
cue at this distance, so this is not a visual pass. The test did not record
frame-time data; no performance conclusion is claimed. Captures are in the
ignored `Playtests/Codex_WindArchAmbientMotes_20261007/Screenshots/` directory.

Follow-up: the tiny (~3.5–5 cm) plain spheres and 140 cm light radius explain
why the event was not legible from the overlook. The effect now uses 14–20 cm
emissive cyan cores, a 600 cm shadow-free light radius, and a pulsing/fading
visual core that tracks its existing eight-second flow. This is still a local
three-light effect. In the matched bounded Game capture, two cyan cores are
clearly visible against the sky in the 17:00 activation frame; they are gone in
the 17:14 frame, consistent with a brief cue. This establishes visibility at
activation, not continuous readability throughout the full effect or a frame-
time cost measurement. The run exited after 60.3 seconds with zero model
requests. The `CaptiveSky2.Agent.NightEcology` automation test passes with
assertions for emissive material assignment, minimum visible scale, and broad
shadow-free local lights.
