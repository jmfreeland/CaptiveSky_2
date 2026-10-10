# A quiet resident can meet the woodland fox (2026-10-10)

An awake Woodland Fox can now notice a nearby resident who approaches slowly
and has a clear view. Within 6 m and at no more than 1.8 m/s, the fox plays its
existing brief look and then trots away from the resident. The response uses
the fox's existing grounded destination selection and stays within its 6.5 m
home patch. Resting and waking foxes do not respond. A response occurs once for
the local group; the encounter rearms only after all residents leave the 8.5 m
area, with an 8-second cooldown as a second guard.

This is deliberately a small wildlife reaction, not a new conscious character
or an instruction to the resident. It writes no memory or world state and makes
no model request. The encounter keeps the fox's routine autonomous: it can
continue foraging or be noticed later by the Raven independently. During the
short glance, a nearby visible resident's next existing situation summary can
report that the fox is looking toward a nearby observer, while explicitly
leaving its intent unknown. The detail expires with the animation and is not
stored as memory.

## Verification

The UE 5.8.3 editor target built from the main project. The
`CaptiveSky2.Agent.WoodlandFox` automation passed with NullRHI, resident
thinking and Python disabled, an isolated world-data root, a 120-second cap,
and zero model requests. It covers range, visibility and movement-speed gates,
the visible resident cue, accurate transient perception without inferred intent
or retained memory, suppression while the same group remains nearby, leaving
the wider range, cooldown, a later re-approach, and a sleeping fox staying
undisturbed. This verifies deterministic encounter and perception state and
the existing animation hookup, not perceived pacing or framing during rendered
play; that needs a bounded Game/PIE observation.

## AutoSDK note

The same day's repeated desktop exception is documented in the [Turnkey
diagnosis](2026-10-07-unreal-mcp-session.md#2026-10-10-standalone-autosdk-check-after-another-popup).
The approved Win64 SDK check succeeded, while the desktop dialog's owning
process and managed stack remain unverified.
