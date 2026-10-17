# RatHollow settlement becomes visible

`RecordWildlifeHomeRestStart` now moves a provisional Rat shelter to `Settled`
after use on three distinct Island days. The visual-only `AIslandWildlifeHome`
previously ignored that state, so a cache could be displayed outside a brand-new
or disturbed hollow and settlement had no corresponding visual payoff.

The materializer now displays the bounded hazelnut cache only for a `Settled`
home. Temporary and disturbed homes keep the cache tucked away. This is a
presentation rule only: the canonical cache record, item quantity, and
last-changed day remain intact, and the component stays non-colliding and out of
navigation. When the world-state record changes, the existing materializer is
refreshed from that canonical record. The current runtime does not yet set a
home to `Disturbed` from a visitor interaction; the disturbed visual is covered
for canonical records and future behavior, not claimed as live player agency.

`CaptiveSky2.Agent.WildlifeHomeRecord` now covers provisional hiding, visible
cache presentation after settlement, persistence/re-materialization, bounded
display count, and hiding after disturbance without deleting stored supplies.
The UE 5.8.3 editor target compiled and linked in the isolated `Claude_Props`
scratch project (5 actions; 7.89 seconds). The focused automation was not run:
project startup is still known to fail with a Core null access before tests
begin, and the editor window is not currently targetable for an in-editor run.
Rendered appearance remains unverified. No Content assets, map data, or model
calls are involved.
