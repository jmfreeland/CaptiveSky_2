# Spectator capture clock and visual check (2026-10-01)

`Config/IslandViewpoints.json` specifies `hour: 17` for its golden-hour camera set, but the Game spectator only read its `viewpoints` array. A first isolated shot tour showed the mismatch: all camera captions remained at 09:00. The editor-backed viewpoint capture has a separate hour option, but the Game launcher had no way to supply it.

`Scripts/Start-Spectator.ps1` now accepts `-ViewpointHour` for shot capture. It is deliberately rejected unless both `-Shots` and an explicit `-DataRoot` are supplied, since the `Island.Hour` filming command changes the active Island clock and the capture should not overwrite the user's normal world state. Example:

```powershell
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking `
  -DataRoot Saved/Playtests/GoldenHour `
  -MaxRealtimeSeconds 120 -MaxModelRequests 1 `
  -ViewpointFile Config/IslandViewpoints.json -ViewpointHour 17
```

UE 5.8.3 verification used the real-RHI Game spectator with separate data and screenshot roots, background agent thinking disabled, a 60-second realtime cap, and a one-request hard ceiling. The 17:00 shot labels and warmer sunset lighting were confirmed across the viewpoints; the run ended normally at 60.2 seconds with zero model requests. A second 20:00 run also ended at its 60-second cap with zero requests and showed the warm guest-book interior after dark. Its selected frame is [the inn guest book at 20:12](../../Saved/Playtests/Codex_NightIsland_20261001/Screenshots/005_Inn_Guest_Book.png); the result is atmospheric, though the page is still blank. The saved [20:00 run log](../../Saved/Logs/Codex_NightIsland_20261001.log) records its enforced cap and zero-request shutdown. Full frames are retained under the corresponding ignored `Saved/Playtests/Codex_GoldenHourTour_20261001` and `Saved/Playtests/Codex_NightIsland_20261001` screenshot directories.

This is a capture-path fix, not a broad art pass. Daylight still exposes flat terrain, unfinished white landmark/blockout forms and a close camera that can clip the inn geometry. The next visual improvement should address the unfinished landmark silhouettes and a cleaner, more grounded inn exterior framing; do not silently alter the user's `Config/IslandViewpoints.json` while doing so.
