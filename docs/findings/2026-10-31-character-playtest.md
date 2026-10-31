# Character playtest — 2026-10-31

## Run

- Island PIE ran for 942.9 real seconds (15m 42.9s), then was stopped cleanly.
- Safety limits were 1,800 real seconds and 64 model requests. The session used 59 requests; all were drained at shutdown.
- Situations were logged, and persistent playtest data was isolated under `Saved/Playtests/Codex_CharacterFeedback_20261031/` rather than the shared live-world state.
- The Unreal log covered editor startup as well as PIE. The PIE interval is the 942.9-second value in `LogAgentSession`.

## What happened

- The chronicle recorded 59 resident decisions: Aster 14, the Innkeeper 13, Rat 15, and Raven 17.
- Raven gathered fallen twigs, then arranged eight stones into a pair at `ArrangingGround_1`. Raven titled it “Two Quiet Marks” and gave the intent: “Leave two stones facing the listening place, like a pause between sounds.” All four residents observed the visible work; none learned its title or intent automatically.
- Rat inspected a pale stone and left it unchanged. Residents sometimes approached one another gently, but the log did not show conversation being forced by proximity.
- Raven later wove the first of five nest layers at `Roost_East`.
- Rat and Raven each set a leaf afloat in the rain basin. The basin and both leaves are present in the isolated world state.
- The world chronicle recorded three emergent trail regions, a rain shower, and a rainbow. The saved Island state ended at about 18:16 on day one, with rain-soaked ground and a light breeze.
- Several early actions were correctly refused or redirected (for example, arranging stones before standing close enough); one movement attempt failed because its target tag was not present.

## Stability and visual notes

- The first editor launch stalled at the Zen startup prompt. Once Zen was available, two editor instances briefly opened; the duplicate was closed. The large video-memory warning disappeared with only one editor remaining.
- After restart, querying `r.Nanite.Foliage` in the editor returned `1` with `LastSetBy: ProjectSetting`.
- During the 15m 43s PIE run, the Unreal process remained responsive. The project log contained no fatal, unhandled-exception, or video-memory-exhaustion entry.
- PIE teardown logged the existing `Unable to find RecastNavMesh instance while trying to create UCrowdManager instance` warning. Project load also logged the landscape edit-layer merge batch-size warning (4033² landscape vs. 1024² configured batch maximum).
- After the run, `CaptiveSky2.Agent.IslandOcean` passed in the editor. `CaptiveSky2.Agent.IslandWeather.BroadleafMixing` also passed. `BroadleafDiversity` reported only two alder cull-distance mismatches: the compiled editor test expected 80/125 m while the loaded component reported 120/220 m. Both the checked-in `IslandWeather.cpp` and `IslandWeatherBroadleafTests.cpp` specify 120/220 m, indicating the live editor's automation binary is stale relative to this checkout; rebuild before treating this as a source regression.
- The night PIE view around the rain basin looked notably lush, with dense low vegetation, floating leaves, an agent, and the water feature. The editor viewport still showed broad open slopes and visibly sparse groundcover; this run does not establish that the whole island now has natural distribution or multiple tree species.
- Walking-on-water behavior was not explicitly tested in this session. Treat that report as open.

## Second bounded preview

- After the user approved the restart, the editor reopened with `r.Nanite.Foliage=1`. A second standalone Island preview ran for 952.0 real seconds (15m 52s), then was stopped from the editor toolbar. It used 19 model requests, with none in flight at shutdown; the safety limits remained 1,800 seconds and 64 requests.
- The second preview used the same isolated playtest root, not the shared live-world state. The chronicle advanced through day two to about 03:18 and continued to record autonomous resident activity.
- Raven described the west roost as exposed to wind and rain, later chose to sleep there, and then decided to walk somewhere less familiar. Rat expressed a preference for a sheltered, familiar hollow. These are direct, actionable signals for a more protective nest and the hidden fallen-wood hollow already chosen for Rat.
- Rat and Raven discussed how gusts made the Listening Stones sound “almost like a chord.” A meteor appeared naturally. Aster approached the rain basin and floated a leaf in it; the isolated basin now recorded three drifting leaves.
- The nighttime standalone view showed dense, layered fern/grass cover around the basin and several distinct tree silhouettes across the middle distance, unlike the editor-only viewport. This is a local composition, not proof of naturally distributed or legible species across the island; the close groundcover still reads dense and clustered in places.
- This was a visual/behavioral observation, not a performance profile: no matched frame-time or whole-device VRAM measurement was collected. The walking-on-water report also remains untested in this run. Keep both open; do not infer that Nanite foliage resolved memory pressure or the ocean collision issue from this preview alone.
- The editor returned to the saved map after Stop. The log reported a clean PIE end with 19 requests and zero still in flight. The session was within its configured limits; it was not left running overnight.

## Evidence

- Unreal log: `D:/Projects - Athena/unreal/CaptiveSky_2/Saved/Logs/CaptiveSky_2.log` (local machine).
- Isolated chronicle, resident memories, trails, and world state: `Saved/Playtests/Codex_CharacterFeedback_20261031/` (local ignored playtest data).
- The repository’s read-only summarizer was run against the completed Unreal log: `Scripts/Summarize-Session.py`.
