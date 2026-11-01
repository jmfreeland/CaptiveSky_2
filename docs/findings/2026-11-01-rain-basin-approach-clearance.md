# Rain-basin approach clearance (2026-11-01)

The 15-minute resident feedback run showed Rat's direct approach to the
rain basin blocked by nearby StarterContent rocks. The AI eventually recovered
with a capsule-clear detour, so the interaction remained possible, but the
basin's placement rule only checked a small circle at the basin itself and
could choose a cramped site.

Island basin placement now checks eight grounded standing positions at 260 cm
around each candidate. A candidate is accepted only when at least two spots
have level ground within 45 cm of the basin and are clear for a 42 cm radius,
90 cm half-height resident capsule. Existing checks for flat basin ground,
center clearance, and nearby navigation projection remain in place. This is a
placement-quality screen, not a route guarantee; the resident's existing
capsule-aware path and recovery logic remains responsible for the actual walk.

The `CaptiveSky2.Agent.IslandRainBasinWorld` automation now covers an open site,
one blocked side (still acceptable), and a ring with only one possible clear
side (rejected). The UE 5.8.3 scratch Editor target compiled successfully,
including both modified source files. The automation has not yet been run:
the open editor is using the separate D: project and the known headless
NullRHI startup crashes make another identical run unreliable. No saved map,
asset, or D: project file was changed. Recheck this placement in a rendered
Island/Game session when the editor and GPU budget allow, and compare resident
approach recovery logs before/after.
