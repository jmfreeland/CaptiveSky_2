# Tideglass minnow school motion (2026-10-16)

The five procedural minnows previously kept fixed, even angular spacing and a
fixed elliptical orbit. Although their color and tails varied, their shared
phase made the group move like one rigid carousel.

`UpdateSchool` now gives each fish a small deterministic radial variation
(±7%) whose phase evolves slowly with the school's existing swim clock. The
school remains coordinated, remains bounded by the measured pool footprint,
and still uses the same population, tide-following height, rain/night tuck,
visitor response, and ripple behavior. No random state, extra actors, or model
requests were added; the adjustment is five inexpensive sine evaluations per
update.

`IslandMinnowTests` checks that one fish's normalized orbital radius changes
over time while every fish remains inside the measured pool footprint, and that
replaying a swim phase reproduces the same position. Static whitespace checks
pass, and the UE 5.8 editor target compiled successfully from an isolated
scratch project. Automation did not start: the scratch NullRHI launch hit the
known project-bootstrap crash before tests began; see the
[isolated validation finding](2026-10-16-scratch-ue-validation.md). Normal-play
visual readability and frame-time impact remain pending.
