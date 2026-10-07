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
full current working set (14 UBT actions). The headless editor automation
`CaptiveSky2.Agent.WindArchPresentation` passed, including the calm/strong-wind
threshold and cooldown assertions, saved-map proxy recognition, and transient
stonework fixture. The automation does not advance natural weather in a Game/PIE
session or capture the mote effect. A short visual/runtime check at calm and
strong wind is still needed to judge readability and frame cost; no gameplay
play session has been run for this change.
