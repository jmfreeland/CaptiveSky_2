# Night fireflies gather near Listening Stones — 2026-10-17

One of the three existing night fireflies now has a stable, elevated home point
within the habitat-facing side of a nearby Listening Stones landmark's 11 m
sound field. The other two retain their separate homes near the habitat. This
preserves the established single-firefly landmark audience rather than crowding
the stones with all three lights. Population, actor tick rate, point-light count,
movement radius, model-request budget, and persisted world state are unchanged.

The placement helper is deterministic and handles an invalid or collapsed
route. The focused `CaptiveSky2.Agent.FireflyCorridorPlacement` test passed under
NullRHI with model requests and world-state writes disabled. The successful
startup used `-DisablePlugins=ModelContextProtocol`; the main project still has
that plugin enabled, so this does not establish that its normal startup is
fixed. The broader `NightEcology` run showed that placing all three fireflies
near the stones violated its explicit one-firefly expectation. The spawner now
keeps only the first near the stones and leaves the other two at their habitat.
The corrected `NightEcology` test passed, including the exact single-listener
assertion. The focused placement test passed after simplifying the helper to
one stable anchor. Both passed with the plugin disabled; the focused placement
test and full `NightEcology` suite also passed in separate runs with the project
MCP plugin enabled.

The revised night composition still has not had visual or play-time review.
Passing placement assertions do not prove that the lights read well among the
authored stones; inspect it in a real-RHI night session before tuning the
spacing.
