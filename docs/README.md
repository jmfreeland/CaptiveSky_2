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
- **Tests:** `Tests/` (`CaptiveSky2.Agent.*` automation, `CaptiveSky2.Visual.*` captures).

## Conventions worth knowing

- Observation is not fact; movement is not consent; a temporary effect is not a lasting change.
- Anything residents are told about the world comes from a pure `Describe...` function so it can be unit-tested without a model.
- `Content/` is untracked; rebuild assets with the scripts in `Scripts/`.
- Check `COORDINATION.md` before editing; commit small and single-purpose.
