# Tidepool minnow nocturnal rest

## Change

The existing Tideglass school now settles into a slower, tighter swimming
pattern as the Island moves from dusk into night, then gradually returns to its
daytime orbit by 08:00. Rest is strongest from 22:00 through 05:00. The school
does not freeze, disappear, sleep on dry land, or lose its existing response to
nearby observers, Raven flybys, and natural surface cues. Minnow surface breaks
become less frequent during the resting period (60 seconds at full rest versus
19 seconds by day).

The weather manager preserves the same school actor across dusk, the full night,
and dawn rather than destroying it at 19:00. Its owner is named for the Tideglass
habitat, not the daytime schedule. This keeps the shallow pool inhabited while
the minnows visibly quiet their swimming rhythm.

This is a lightweight behavioral use of the existing Island clock: no new
assets, timers, model requests, or persistent state were added. Movement and
surface-break scheduling remain within the current actor tick and existing
bounded cue logic.

## Validation

`CaptiveSky2.Agent.NightEcology` now covers dusk/dawn transitions, midnight
wrap, invalid clock values, nonzero minimum activity, slower integrated swim
phase, longer nighttime surface-break intervals, and the actual school-orbit
contraction in a transient world. The first focused run exposed a fixture
lifecycle error (direct `BeginPlay()` before actor startup) and three assertions
showing that the overnight curve dropped back to zero after 22:00. The fixture
now uses `DispatchBeginPlay()`, and the curve uses separate dusk/dawn transitions
bounded to their respective intervals. After rebuilding, the focused test
passed under NullRHI with thinking disabled and zero model requests
([log](../../Saved/Logs/Codex_NightEcology_RestCurveFix_20261013.log)). This is
deterministic behavior evidence, not visual confirmation; the resting school
still needs review under night lighting to ensure it remains subtly alive.

The separate `CaptiveSky2.Agent.TidepoolMinnows` and
`CaptiveSky2.Agent.TidepoolMinnowWaterPresence` tests now cover the manager's
same-actor continuity through dusk, 23:00, midnight, and dawn, alongside the
school's nonzero-but-slower swim step and existing swimmer response. Both passed
after a successful UE 5.8.3 editor-target rebuild; the run used NullRHI,
disabled thinking, and a zero-request ceiling
([log](../../Saved/Logs/Codex_MinnowOvernightPersistence_20261013.log)).

Four bounded real-RHI Game captures sampled the requested Island hours with the
minnow school visibly present in each; each run ended normally with zero model
requests at the configured 60-second cap. The pool and school are visible in the
[`17:00`](../../Playtests/Codex_MinnowOvernight_20261013/Hour17/Screenshots/001_Tideglass.png),
[`23:00`](../../Playtests/Codex_MinnowOvernight_20261013/Hour23/Screenshots/001_Tideglass.png),
[`03:00`](../../Playtests/Codex_MinnowOvernight_20261013/Hour03/Screenshots/001_Tideglass.png),
and [`07:00`](../../Playtests/Codex_MinnowOvernight_20261013/Hour07/Screenshots/001_Tideglass.png)
frames. The school reads as small fish in the broad Tideglass composition; the
captures prove continued presence, not polished close-up readability or a
30-FPS traversal profile. Per-hour logs are under `Saved/Logs/` as
`Codex_MinnowOvernight_Hour{17,23,03,07}_20261013.log`.

## Next step

The close moonlit MinnowSchool view was attempted on 2026-10-13 with an
isolated 03:00 start, zero model requests, and a dedicated actor-anchored camera.
Unreal crashed at zero seconds before world-ready and produced no project log or
screenshots; the crash has the same null-address stack hash as the crab-view
attempt. See [`2026-10-13-unreal-startup-triage.md`](2026-10-13-unreal-startup-triage.md).
Do not repeat the same startup experiment until matching symbols or materially
changed startup conditions are available.

When a real-RHI run is possible again, use the saved close-view config to judge
whether the small school reads clearly against moonlit water, then review the
nighttime water/terrain contrast. Keep that art/readability pass separate from
a 30-FPS traversal profile and retain the existing play/request safeguards. The
four earlier wide captures confirm presence and lighting states, not close-up
readability or target-hardware performance.
