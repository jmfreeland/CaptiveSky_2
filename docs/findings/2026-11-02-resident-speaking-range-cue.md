# Make resident proximity cues accurate (2026-11-02)

The seasonal feedback run showed Aster repeatedly selecting Raven's approach
target. The movement result eventually said Aster was already at a reachable
ground position near Raven. The situation summary still used the generic
"you may approach them" wording for every resident inside its 25-metre
observation radius, even when the pair was already within the same social
component's speaking radius. A repeated `move_to` therefore remained an
apparently valid model choice.

The summary now adds a factual, distance-aware cue when the other resident is
already within normal speaking range: another approach is unnecessary, `Speak`
is available if contact is wanted, and remaining quiet or choosing another
activity is equally valid. The existing cooldown warning still governs when
speech may occur. No extra think cycle, model request, automatic greeting,
social reservation, or conversation turn was added.

`CaptiveSky2.Agent.CraftTeaching` now asserts the cue using two fixture
residents already inside speaking range and verifies that quiet alternatives
remain explicit. The source and test require an Editor build; automation and a
rendered resident encounter remain gated on resolving the known early UE
project-open access violation. Re-run a bounded 15-minute resident session
after that gate to see whether an approach now leads to an optional greeting
or a natural choice to stay quiet.
