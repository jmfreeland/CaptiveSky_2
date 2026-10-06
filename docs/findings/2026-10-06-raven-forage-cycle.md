# Raven forage renewal (2026-10-06)

The fallen-twig bundles at empty arranging grounds are now a small renewable resource rather than session-local presentation. A grounded raven can gather at most one bundle from a site per Island day. The source site's `forage_gathered_day` is saved in the per-map world-state JSON before the visible pile is removed; if saving fails, the gather is refused and the resource remains available. Depletion survives a restart on the same Island day, and the pile is shown again when the saved Island day advances.

An arranging ground that becomes a stone work no longer offers forage. Gathering creates one factual `forage` chronicle event with the raven identity and site; it does not add model calls, alter the nest, claim ownership, or force the raven to build. Existing arrangement records without `forage_gathered_day` load with the default of zero, so their empty sites begin available.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` target build succeeded after the source changes.
- `CaptiveSky2.Agent.IslandNest` passed in a headless `UnrealEditor-Cmd` run with `-NullRHI`, `-DisablePython`, and no provider/model request. The expanded fixture verifies visible depletion, duplicate-gather refusal, one renewal after an Island-day change, depletion after reopening the same day, and renewal after a later day.
- Log: `Saved/Logs/Codex_RavenForageRenewal_Final_20261006.log`.

This validates the deterministic resource/state loop, not whether a live raven chooses to forage, how the props look in the saved Island, or the experience in a packaged build. Those remain useful next checks when the editor is available.
