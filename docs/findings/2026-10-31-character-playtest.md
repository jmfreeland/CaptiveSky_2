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
- The night PIE view around the rain basin looked notably lush, with dense low vegetation, floating leaves, an agent, and the water feature. The editor viewport still showed broad open slopes and visibly sparse groundcover; this run does not establish that the whole island now has natural distribution or multiple tree species.
- Walking-on-water behavior was not explicitly tested in this session. Treat that report as open.

## Evidence

- Unreal log: `D:/Projects - Athena/unreal/CaptiveSky_2/Saved/Logs/CaptiveSky_2.log` (local machine).
- Isolated chronicle, resident memories, trails, and world state: `Saved/Playtests/Codex_CharacterFeedback_20261031/` (local ignored playtest data).
- The repository’s read-only summarizer was run against the completed Unreal log: `Scripts/Summarize-Session.py`.
