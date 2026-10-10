# A quiet resident can meet the woodland fox (2026-10-10)

An awake Woodland Fox can now notice a nearby resident who approaches slowly
and has a clear view. Within 6 m and at no more than 1.8 m/s, the fox plays its
existing brief look and then trots away from the resident. The response uses
the fox's existing grounded destination selection and stays within its 6.5 m
home patch. Resting and waking foxes do not respond. A response occurs once for
the local group; the encounter rearms only after all residents leave the 8.5 m
area, with an 8-second cooldown as a second guard.

If a resident group arrives while a wildlife-initiated glance is already in
progress, the fox now treats them as part of that same encounter. It latches the
existing local-group guard and keeps the original retreat destination instead
of being redirected toward a second nearby observer on the next presence check.
The normal resident response to an already-foraging fox is unchanged.

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
play; the encounter was invoked directly by the rendered fixture below, so this
is not proof of its autonomous encounter frequency.

The follow-up UE 5.8.3 build and `CaptiveSky2.Agent.WoodlandFox` regression also
passed with thinking disabled and zero requests. The new case stages a nearby
resident group during an active wildlife glance and verifies that it is latched
without changing the retreat target; the separate `CaptiveSky2.Agent.RavenPerch`
regression passed unchanged afterward, preserving the Raven's staged glance and
the fox's bounded retreat.

Evidence: [`WoodlandFox group-latch regression`](../../Saved/Playtests/Codex_WoodlandFox_GroupLatch_20261010/WoodlandFox.log),
[`RavenPerch compatibility regression`](../../Saved/Playtests/Codex_RavenPerch_GroupLatchRegression_20261010/RavenPerch.log).

### Respectful wildlife movement boundary (2026-10-10)

A bounded UE 5.8.3 real-D3D12 probe confirmed that `WoodlandFox` is not a valid
resident movement target. Aster's requested `MoveTo` was rejected with the
ordinary safeguard: wildlife are not movement targets; residents may observe
from a respectful distance or interact quietly when already close, but do not
chase, feed, touch, or claim them. No movement or fox encounter occurred in
that probe; it exited normally after 29.9 real seconds with zero model requests.
UBT AutoSDK validation returned 0 on this approved launch.

This is the intended independence boundary, not a movement regression. Fox
resident-notice behavior should be validated through an incidental slow
pass-by or an already-nearby quiet interaction, not by directing a resident at
the fox. Log: [`FoxEncounterPhysical.log`](../../Saved/Playtests/Codex_FoxEncounterPhysical_20261010/FoxEncounterPhysical.log).

### Rendered staging

`CaptiveSky2.Visual.WoodlandFoxResidentGlance` passed in UE 5.8.3 on D3D12 with
resident thinking and Python disabled, an isolated data root, a 120-second
realtime ceiling, and zero model requests. The fixture stages transient Aster
and fox actors at the logged runtime woodland site, orients Aster toward the
fox, starts the fox's existing brief look, and checks the same immediate
situation summary. It uses the project's bounded editor foliage preview
(1,780,640 ground-cover instances, 14,727 trees, 15,063 shrubs, and 1,003
meadow flowers) and clears that preview automatically afterward; no level or
world-state file is saved.

The real-RHI frame is
[`01_Aster_Fox_WoodlandMeadow.png`](../../Saved/Viewpoints/WoodlandFoxResidentGlance_/20261010_041957/01_Aster_Fox_WoodlandMeadow.png);
the run log is
[`FoxResidentGlance.log`](../../Saved/Playtests/Codex_FoxGlance_FinalFraming_20261010/FoxResidentGlance.log).
It shows the glance with both figures unobstructed in the populated meadow,
but the Wind Arch's white/blue blockout pillars are still prominent. Treat this
as interaction and foliage evidence, not a finished beauty shot or proof that
the event naturally occurs at this camera position.

## AutoSDK note

The same day's repeated desktop exception is documented in the [Turnkey
diagnosis](2026-10-07-unreal-mcp-session.md#2026-10-10-standalone-autosdk-check-after-another-popup).
The approved Win64 SDK check succeeded, while the desktop dialog's owning
process and managed stack remain unverified.
