# Raven wildlife notice range

## Change

In `ARavenAgentAIController::CheckForNearbyWildlifePresence`, the Raven's wildlife
notice radius is now 10 m (previously 5 m); the nearby-group forget radius is 14 m
(previously 7 m). The existing line-of-sight, settled/perched, awake/resting, and
wildlife-specific eligibility checks remain in place. The Raven still gives a brief
look rather than pursuing the animal, while an awake fox performs its existing
bounded retreat. Sleeping foxes remain undisturbed. No model request is involved.

The doubled radius is intentionally local: it gives a Raven that passes through the
same patch a better chance to notice wildlife without pulling either animal across
the island.

## Evidence

- The 90-second isolated natural-play observation on 2026-10-10 recorded the
  transient fox at `(-100915.04, 97967.89, 2869.30)`. The spectator's west-roost
  anchor resolved to `(-100640, 101250, 3079.11)`, about 32.9 m away horizontally.
  The run did not log a Raven/fox exchange. This short, thinking-disabled sample
  shows no encounter; it does not establish long-run encounter frequency.
- `CaptiveSky2.Agent.RavenPerch` passed in the isolated UE 5.8.3 `Claude_Props`
  scratch project using NullRHI, `-CaptiveSkyDisableAgentThinking`, a zero-model-
  request budget, and a 240-second realtime watchdog. It checks a close fox's
  animated glance and bounded retreat, a clear-sighted fox 8.5 m away (outside the
  old 5 m limit) being noticed, a resting fox being ignored, and a clear-sighted
  fox outside the 10 m notice radius remaining undisturbed.
- The scratch `CaptiveSky_2Editor` target built and linked successfully with the
  UE 5.8 toolchain installed at `D:\Games\Epic\UE_5.8`.

## Limit and next step

This is a perception opportunity, not a guaranteed encounter. The observed west
roost and fox spawn are still about 33 m apart, and tree cover can block sight.
Next, capture longer bounded no-request play with low-cost wildlife/roost telemetry
to see whether normal Raven movement brings it into the fox's patch. Use those
observations before changing habitat placement or Raven flight behavior.
