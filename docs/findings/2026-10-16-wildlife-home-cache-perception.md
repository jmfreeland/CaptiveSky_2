# Rat residents can perceive their recorded home cache — 2026-10-16

When an assigned wildlife home is within the existing 25 m, line-of-sight
perception range, its occupant now receives the canonical hazelnut quantity in
the next situation summary. An empty hazelnut cache is stated as unrecorded.
This cue is restricted to the matching occupant; other residents receive
neither the home's movement target nor its stored contents. It does not add a
new action, move the resident, alter the cache, create a model request, or
assert that stored nuts are fresh or edible. Cache display and the record
remain the source of truth; the prompt adds awareness, not a new food system.

## Validation

- UE 5.8.3 scratch editor build succeeded; only `AgentBrainComponent.cpp` and
  `IslandInnRestTests.cpp` needed compilation before linking.
- `CaptiveSky2.Agent.IslandInnRest` passed under NullRHI with Python disabled
  and `-CaptiveSkyMaxModelRequests=0` in the isolated `Claude_Props` scratch
  project ([log](../../Saved/Logs/Codex_WildlifeHomeCachePerception_20261016.log)).
- Regression coverage checks the occupant sees the stored count without an
  edibility/freshness claim, does not see it beyond the existing 25 m range,
  and another resident cannot see it.
- No saved Island, Rat home placement, resident memory, or model-request state
  was changed. The result validates prompt construction in a synthetic fixture,
  not an autonomous Rat's choice or in-world presentation.
