# Bounded Island highlight captures (2026-10-06)

Two isolated golden-hour spectator captures were run on UE 5.8.3. Both loaded `/Game/Maps/Island`, disabled resident thinking, used a one-request ceiling, and wrote world data beneath `Saved/Playtests/` rather than the normal resident data root. Both game processes exited normally at their real-time safety limits; the first was capped at 180 seconds and the second at 120 seconds. No model requests were enabled by the capture runs.

The first pass used the viewpoint file's 24-second default. It captured seven views before its cap. The second passed `-EstablishingSeconds 8` to the spectator script, captured the nine-view sequence, and then repeated views while the bounded session ran to its cap. This is useful for future review runs: shorten each view interval when the goal is a complete gallery, but leave enough time after startup for screenshots to flush.

Local captures:

- `Saved/Playtests/Codex_Highlight_20261006_2230/Screenshots/` — first pass, seven frames.
- `Saved/Playtests/Codex_HighlightFast_20261006_2220/Screenshots/` — faster pass, thirteen frames (the configured nine viewpoints plus four repeats).
- Logs: `Saved/Logs/Codex_Highlight_20261006_2230.log` and `Saved/Logs/Codex_HighlightFast_20261006_2220.log`.

The strongest single composition in this review is `008_Wind_Arch_Overlook.png`: sunset light, the grove and ocean, the Wind Arch, and a small resident-scale figure share the frame. `007_Listening_Stones.png` is a good alternate with stronger sky and tree silhouettes. The warm `002_Inn_From_Path.png` clearly frames the inn but makes its blockout proportions and oversized foreground sign especially visible.

The captures also surface follow-ups rather than hiding them. The Tideglass water reads as a dark, nearly featureless patch in the late-day frames, consistent with the earlier Water-readability finding. In the first pass, after the clock advances beyond 18:00, Tideglass/ground-detail frames become extremely dark and show an on-screen warning that multiple directional lights are competing for forward shading. That makes dusk/night composition a poor highlight until the sun/moon light-priority and exposure behavior are understood. The shoreline overview also shows sparse, widely spaced tall silhouettes; close vegetation density does not yet produce a convincing middle-distance grove.

Both standalone Game logs also contain Python `AttributeError` traces from UE 5.8 experimental editor-toolset startup scripts trying to access editor-only Python types such as `ToolsetDefinition` and `PythonTestRunner`. The captures continued; these traces are separate from game-code automation failures and are not evidence of the user's earlier `dotnet.exe` exception dialogs. They were not changed as part of this capture pass.

Next visual work: investigate the directional-light warning and actual exposure transition first; then recapture Tideglass at midday and dusk using the same camera to compare its surface and wet-edge readability. Treat the broad-view tree spacing as a separate ecological composition issue, not a reason to blindly raise the total foliage count—the gameplay profiling notes identify a substantial existing foliage render cost.
