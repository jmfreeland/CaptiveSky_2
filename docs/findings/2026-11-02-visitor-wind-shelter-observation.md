# Visitor wind-shelter observation — 2026-11-02

Visitors using the on-demand `E: notice your surroundings` cycle can now
receive a local wind-shelter note when ambient wind is strong enough to assess.
It reuses `AIslandWeather::DescribeWindShelterAt` and its six-metre upwind
visibility trace, so the observation reflects solid geometry at the visitor's
current position rather than nearby landmark names. It only distinguishes
present horizontal-wind exposure from a blocked upwind trace; it explicitly
does not claim roof cover, waterproofness, or a safe perch/nest site. No model
call, memory write, or world change is involved, and calm conditions add no
wind-shelter entry to the observation cycle.

## Validation

The UE 5.8.3 `CaptiveSky_2` Game target built and linked successfully with a
one-worker cap. The build compiled both the new controller helper and the
focused automation source, including cases for an upwind blocker, an exposed
site after removing the blocker, and the roof/perch caveat. Automation runtime
and the rendered player caption have not been verified; the active editor was
left open. The next check is the focused `VisitorWeatherObservation` automation
in a safe isolated Editor session, followed by a playable Island caption check.
