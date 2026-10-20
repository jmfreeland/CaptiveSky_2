# CaptiveSky documentation hub

Start here. A visual overview lives in [`hub/index.html`](hub/index.html) (open it in a browser);
this file is the text version and the map of everything under `docs/`.

## Reading order

1. [`../README.md`](../README.md): vision, governing principle, dated design record for every system.
2. [`hub/index.html`](hub/index.html): the world and the code, drawn on one page.
3. [`../COORDINATION.md`](../COORDINATION.md): how Claude Code and Codex share the working tree.
4. The folders below, as needed.

## Folders

| Path | What is in it |
|---|---|
| `ecology/` | One note per living system (Tideglass water, minnows, dragonflies, shore grass, meadow flowers, rain basin). |
| `residents/` | Drafts and active profiles for residents (rat, innkeeper). |
| `plans/` | Forward plans, currently the inn. |
| `findings/` | Dated, evidence-based reviews and test reports (the project's lab notebook). |
| `worldbuilding-ideas.md` | Weekly idea branches and their verification notes. |
| `technical-ideas.md` | Engineering ideas not yet built. |
| `asset-import-pipeline.md`, `asset-request-workflow.md` | How art gets into `Content/`. |

## Code map (`Source/CaptiveSky_2/Agent/`)

- **Residents:** `Agent*` (brain, memory, relationships, social, consolidation, LLM provider/settings/model tier, play-session budgets), `AutonomousAgent*`, `RavenAgentAIController`, `RatAgentAIController`.
- **Clock and weather:** `IslandDayNight` (sun, moon phases, starlight), `IslandWeather`, `IslandEnvironmentSubsystem` (shared wetness/wind/mist values, `MPC_IslandEnvironment`).
- **Atmosphere extras:** `IslandDew`, `IslandRainbow`, `IslandGrade`, `IslandLightning`, `IslandDrift`, `IslandFirefly`.
- **Water and shore:** `IslandOceanSubsystem`, `IslandSeaStateSubsystem`, `IslandTideglass*`, `IslandTidepool*`, `IslandWrack`, `IslandRainBasin`.
- **Wildlife:** `IslandForestFox`, `IslandForestStag`, `IslandTideglassDragonfly`, `IslandWildlifeHome`, `IslandNest`.
- **Lasting change:** `IslandWorldStateSubsystem` (persists to `WorldState/<Map>.json`), `IslandArrangement`, `IslandTrail`, `IslandChronicle`, `IslandGuestBook`, `IslandCurio`.
- **Places:** `IslandInnHearthSubsystem`, `IslandInnkeeperSubsystem`, `IslandListeningStone*`, `IslandWindArch*`.
- **Tooling and presentation:** `IslandSpectator` (camera for unattended captures), `IslandInstancedSkinnedFoliage` (skinned tree instances), `IslandMovementProbe` / `IslandNavProbe` (movement and navigation probes), `IslandAssetRequestQueue` (residents requesting art), `AgentDebugCommands`.
- **Tests:** `Tests/` (`CaptiveSky2.Agent.*` automation, `CaptiveSky2.Visual.*` captures).

## What residents are told about the sky

`AIslandDayNight::DescribeTime()` joins small pure functions (all covered by `CaptiveSky2.Agent.DayNight`):
`LunarPhaseName` (eight named phases), `MoonHeight` (high / up / horizon / below), `IsMoonHiddenByCloud`,
`DescribeSunEdge` (sunrise and sunset cues) and `DescribeCalendar` (Island day, days to full and new moon) and `IsShootingStarSlot` (rare meteors on clear dark nights), `DescribeStars` (crowded, drowned by a bright moon, or hidden by cloud) and `TideLevel` / `DescribeTide` (12.42 h tide, spring near new and full moon, neap at the quarters). The sea is described by `UIslandSeaStateSubsystem::DescribeSea`.
Developer console: `Island.Hour <h>`, `Island.Day <n>` (day 1 is a new moon), `Island.SkyReport` (logs what a resident would be told about sky and sea), `Island.Storm`, `Island.Dew`, `Island.Sea`, `Island.Rainbow`.

## Building and testing with the editor open

Use a scratch project whose `Source/` is a junction to the main tree (`Saved/CompileScratch/Claude_Props`):

```
Build.bat CaptiveSky_2Editor Win64 Development -Project=<scratch>\CaptiveSky_2.uproject -WaitMutex -MaxParallelActions=3 -NoHotReloadFromIDE
UnrealEditor-Cmd.exe <scratch>\CaptiveSky_2.uproject -ExecCmds="Automation RunTests CaptiveSky2.Agent.DayNight; Quit" -NullRHI -NoSound -DisablePython -unattended
```

Cold build is about seven minutes. Tests are headless and make no model requests.

## Recent sky work (commits `84b6442`..`5eacd9c`)

Named lunar phases, the moon's position and cloud cover, sunrise and sunset cues, an Island calendar, shooting stars and
the sea state all reach residents through pure, tested functions; `Island.Day` and `Island.SkyReport` were added for
checking them in game. The chronicle (`WorldState/chronicle.jsonl`, read with `Scripts/Build-Chronicle.py`) also records the moon changing phase. Along the way the headless suite went from 8 failures to 4 (see below); a full rerun in cycle 27 showed exactly those 4.

## Test health

See [`findings/2026-10-18-agent-suite-headless-baseline.md`](findings/2026-10-18-agent-suite-headless-baseline.md):
the headless `CaptiveSky2.Agent` suite had 79 passes and 8 failures; four were test bugs and are fixed. Four
(Innkeeper spawn, Ocean wake, Listening Stones, Wind Arch) await a run in the main
project's own automation environment.

## Conventions worth knowing

- Observation is not fact; movement is not consent; a temporary effect is not a lasting change.
- Anything residents are told about the world comes from a pure `Describe...` function so it can be unit-tested without a model.
- `Content/` is untracked; rebuild assets with the scripts in `Scripts/`.
- Check `COORDINATION.md` before editing; commit small and single-purpose.
