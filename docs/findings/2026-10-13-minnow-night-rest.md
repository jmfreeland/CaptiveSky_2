# Tidepool minnow nocturnal rest

## Change

The existing Tideglass school now settles into a slower, tighter swimming
pattern as the Island moves from dusk into night, then gradually returns to its
daytime orbit by 08:00. Rest is strongest from 22:00 through 05:00. The school
does not freeze, disappear, sleep on dry land, or lose its existing response to
nearby observers, Raven flybys, and natural surface cues. Minnow surface breaks
become less frequent during the resting period (60 seconds at full rest versus
19 seconds by day).

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

## Next step

Run a short, zero-model-request Tideglass Game capture at 17:00, 23:00, 03:00,
and 07:00 when the UE 5.8.3 session is stable. Confirm that
daytime minnow activity is unchanged, night movement remains visible, and the
existing surface-cue and Raven-disturbance behaviors still work before treating
this as visually verified.
