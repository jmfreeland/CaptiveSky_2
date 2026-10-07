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
session. A short live visual check at calm and strong wind is still needed to
judge readability and frame cost; runtime appearance remains unverified.
