# Rat home cache visual materialization — 2026-10-16

`FIslandWildlifeHomeRecord` already stores a bounded list of canonical cached
supplies, but the transient `AIslandWildlifeHome` materializer previously showed
only the RatHollow shelter. It now maps the existing `Hazelnut` item ID to the
Common Hazel nut-branch asset and materializes up to four small, deterministic
instances beside the hollow entrance. The persisted quantity remains exact;
the four-instance cap is presentation-only. Other item IDs remain valid canonical
data but are not displayed until a visual mapping is authored for them.

The cache component is visual-only: collision, overlap generation, navigation
influence and shadow casting are disabled. The home record remains the source of
truth, instances are regenerated when it is applied, and removing a supply clears
the old instances. No Island map placement or authored world-state file was
changed.

`CaptiveSky2.Agent.WildlifeHomeRecord` now asserts the imported mesh path, three
visible items for quantity three, the four-item display cap for a full canonical
cache, cache removal/restoration, and no collision or navigation influence.
The UE 5.8.3 isolated scratch target compiled and linked successfully (7 actions,
including `IslandWildlifeHome.cpp` and `IslandNestTests.cpp`). After tightening
the cache assertions and grounding the instance transforms from the mesh bounds,
the final incremental rebuild also succeeded (4 actions; the actor compiled and
the module relinked).

The automation test was not run. The current project-startup finding records a
reproducible zero-second null-access crash in the unsymbolized Unreal Core
startup path, before any project module is loaded, and explicitly gates unchanged
project-opening retries until matching symbols are available. This milestone is
therefore compile-verified but not runtime-verified. Next: symbolize the retained
UE 5.8.3 minidump, then run `CaptiveSky2.Agent.WildlifeHomeRecord` in a controlled
session and inspect the cache at rat height before treating its composition as
visually approved.
