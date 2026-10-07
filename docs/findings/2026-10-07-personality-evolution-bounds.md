# Bound sleep-based personality evolution (2026-10-07)

Sleep consolidation already limited each rest to three small changes, but it
did not limit the lifetime tendency list or the size of an individual reason
and evidence list. The reflection prompt also omitted the resident's existing
tendencies, making stable trait names harder to reuse across sleeps.

The derived overlay is now bounded to 12 distinct tendencies for the agent's
lifetime. Each tendency keeps at most 256 characters of explanation and three
supporting memory IDs of at most 128 characters each. Oversized explanations
and new traits after the lifetime cap are ignored; existing traits can still
evolve. Loading an older overlay applies the same bounds in memory, trims names,
rejects names over 64 characters and evidence IDs over 128 characters, and
deduplicates names case-insensitively, while the append-only history remains
the recovery record. The sleep prompt now includes current tendency names and
strengths, encourages exact name reuse, and makes the limits explicit. The
authored identity and personality documents remain immutable, and the existing
per-sleep adjustment and model-request caps are unchanged.

## Verification

The UE 5.8.3 `CaptiveSky_2Editor` Development target built successfully with
UHT and a full module relink (4 UBT actions). The focused
`CaptiveSky2.Agent.PersonalityConsolidation` automation passed. It covers prompt
reuse guidance, the 12-tendency lifetime cap, rejection of an overlong
explanation, the three-ID evidence cap, and bounding an oversized legacy
overlay, including rejection of an overlong trait name, case-insensitive
duplicate names, and overlong evidence IDs. The test ran with NullRHI, Python
and agent thinking disabled, a zero model-request cap, and an isolated data
root; it does not establish what a live model will choose to remember during
sleep. Log: [`Codex_PersonalityEvolutionBounds_20261007.log`](../../Saved/Logs/Codex_PersonalityEvolutionBounds_20261007.log).

A live-model sleep remains a separate, explicitly budgeted check; the code
change itself adds no sleep call, model request, or automatic memory.
