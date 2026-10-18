# Headless `CaptiveSky2.Agent` suite baseline (2026-10-18)

Run from the scratch project `Saved/CompileScratch/Claude_Props` (its `Source/` is a junction to the main tree, so it
includes other agents' uncommitted edits as of commit `ab59781`), with `-NullRHI -NoSound -DisablePython`, thinking
disabled and no model requests:

```
UnrealEditor-Cmd.exe <scratch>\CaptiveSky_2.uproject -ExecCmds="Automation RunTests CaptiveSky2.Agent; Quit" -NullRHI -NoSound -DisablePython -unattended
```

Result: **79 passed, 8 failed.** `DayNight`, `DayNightPersistence` and `IslandSeaState` (the suites touched by this
session's sky-text work) pass. None of the failures involve code changed in this session, but I did not run the suite
on the parent commit, so "pre-existing" is an inference from the failing assertions, not a measurement.

| Test | First failing assertion |
|---|---|
| `IslandDewRuntime` | **Fixed** (test assumed three settling passes; the 180-glint pool needs four at 48 seats per tick, so it now settles in a bounded loop) |
| `IslandInnkeeperSpawn` | The independent identity / personality documents load from their own agent folder |
| `IslandOcean` | Moving across the ocean leaves one short water wake (expected 1, was 0) |
| `ListeningStonePresentation` | The staged visitor is player-controlled |
| `NightEcology` | The periwinkle fully re-extends when the deterministic weather returns dry |
| `TidepoolMinnows` | The school visibly widens rather than fleeing the pool (plus four later assertions) |
| `TidepoolPeriwinkleNocturnalGrazing` | **Fixed** (test bug: hour 24 wraps to hour 0, so it should equal night activity, not midday) |
| `WindArchPresentation` | The test visitor is controlled by a player controller |

Patterns worth checking before treating these as regressions:
- Four tests (`IslandOcean`, `ListeningStonePresentation`, `WindArchPresentation`, and the wake check) depend on a
  player controller possessing a staged character; a headless `-NullRHI` run without Content/a game viewport may not
  provide one.
- `IslandInnkeeperSpawn` reads `Agents/Agent_Innkeeper_01/`; the scratch project has its own `Agents/` folder.
- The periwinkle failure turned out to be a test bug (see the table), not float rounding.

Next step for whoever owns each area: rerun the failing test in the main project's own automation environment to
separate environment artefacts from real regressions.
