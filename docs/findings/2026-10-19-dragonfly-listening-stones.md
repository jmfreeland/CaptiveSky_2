# Dragonflies investigate a Listening Stones chime

Nearby Tideglass dragonflies now make a brief, non-persistent flight adjustment when they hear a Listening Stones tone. Each color morph takes a slightly different hover station 58 cm around and 125 cm above the stones, adding a visible ecological answer to a visitor-operated landmark without making it a puzzle or promised discovery.

The response lasts up to 2.6 seconds and has an 18-second cooldown. It does not interrupt a flower visit, ripple investigation, rain-basin visit, or startle response; nearby ripples and startles cancel it. Heavy rain suppresses starting the response. It changes no landmark, memory, saved map, or resident request state.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded with one worker (15 actions, including `IslandTideglassDragonfly.cpp` and the new automation test).
- `CaptiveSky2.Agent.Ecology.DragonflyListeningStonesResponse` passed in the isolated scratch project with `-NullRHI`, thinking disabled, a 60-second cap, and zero model requests. The test checks audible-chime discovery, station spacing/height, movement toward the temporary station, same-chime deduplication, preservation of an active flower destination, and rejection of an out-of-range chime. Log: `Saved/CompileScratch/Codex_TideglassPollinator_20261019/Project/Saved/Logs/Codex_DragonflyListeningStones_20261019.log`.
- The main Unreal editor and saved Island were not modified. The automation validates deterministic behavior, not rendered flight readability; a real-RHI visual check remains pending.
