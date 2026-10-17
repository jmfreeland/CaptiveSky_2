# Night fireflies gather near Listening Stones — 2026-10-17

The three existing night fireflies now occupy stable, staggered home points on
the habitat-facing side of a nearby Listening Stones landmark. Their anchors are
kept within the landmark chime's 11 m sound field, giving all three an opportunity
to hear a fresh resonance; the short lateral and height offsets keep the trio
from starting in one overlapping cluster. Population, actor tick rate,
point-light count, movement radius, model-request budget, and persisted world
state are unchanged.

The placement helper is deterministic and handles an invalid or collapsed
route. The UE 5.8.3 scratch editor target linked successfully (35 actions,
72.52 s), then `CaptiveSky2.Agent.FireflyCorridorPlacement` passed under
NullRHI with model requests and world-state writes disabled. The successful
startup used `-DisablePlugins=ModelContextProtocol`; the main project still has
that plugin enabled, so this does not establish that its normal startup is
fixed. The broader `NightEcology` test ran in the same session but failed four
shore-crab player-presence assertions; those failures do not involve firefly
placement and are recorded separately in the project-bootstrap finding.

The revised night composition still has not had visual or play-time review.
Passing placement assertions do not prove that the lights read well among the
authored stones; inspect it in a real-RHI night session before tuning the
spacing.
