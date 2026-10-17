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

`CaptiveSky2.Agent.WildlifeHomeRecord` covers provisional hiding, visible cache
presentation after settlement, persistence/re-materialization, bounded display
count, and hiding after disturbance without deleting stored supplies. The
current-source test passed in the isolated
`Codex_VisitorResonance_20261017` scratch project under NullRHI, with agent
thinking disabled, zero model requests, a 120-second cap, and the Model Context
Protocol plugin disabled (exit 0):
`Saved/CompileScratch/Codex_VisitorResonance_20261017/Saved/Logs/Codex_WildlifeHomeRecord_20261017.log`.
This validates the canonical state/materializer contract and the imported mesh
reference, not the mesh's appearance, entrance readability, collision in-game,
or Rat-scale traversal. Real-RHI placement review remains necessary. No map data
or model calls are involved.
