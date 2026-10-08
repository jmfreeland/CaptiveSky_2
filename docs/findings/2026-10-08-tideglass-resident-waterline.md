# Tideglass waterline as a nearby observation (2026-10-08)

Nearby residents now receive a factual Tideglass waterline description in their
situation summary when the pool landmark is visible and an Island clock exists.
The observation reports near-mean water, or an approximate centimetre offset
above/below mean with a near-high/near-low or between-turn description. It is
context only: it does not make the resident move, interact, call a model, or
persist a new memory.

The focused `CaptiveSky2.Agent.TideglassTide` automation passed in an isolated
UE 5.8.3 scratch project. It exercises the same resident situation-summary
builder at spring high and low water and retains the existing waterline-bound
checks. The scratch editor target also built successfully with the user's live
editor open; no live editor state was saved or changed. The first fixture run
omitted the `IslandLandmark` tag required by production visibility filtering;
the fixture was corrected and the rerun passed.

This validates the observation's construction and tide values, not that a
resident will spontaneously choose to visit Tideglass or that every saved-map
landmark is visible from every position.
