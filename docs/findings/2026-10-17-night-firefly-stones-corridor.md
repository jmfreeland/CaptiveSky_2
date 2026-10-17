# Night fireflies gather near Listening Stones — 2026-10-17

The three existing night fireflies now occupy stable, staggered home points on
the habitat-facing side of a nearby Listening Stones landmark. Their anchors are
kept within the landmark chime's 11 m sound field, giving all three an opportunity
to hear a fresh resonance; the short lateral and height offsets keep the trio
from starting in one overlapping cluster. Population, actor tick rate,
point-light count, movement radius, model-request budget, and persisted world
state are unchanged.

The placement helper is deterministic and handles an invalid or collapsed
route. `CaptiveSky_2Editor` compiled the changed weather runtime and ecology
regression test with UE 5.8.3 `-NoLink` (12 actions, 57.52 s). The project-opening
NullRHI crash remains a known blocker to automation execution, and the revised
night composition has not yet had visual or play-time review. Compilation and
placement assertions do not prove that the lights read well among the authored
stones; inspect it in a real-RHI night session before tuning the spacing.
