# Minnow response to a visitor's swim wake

## Change

Tideglass minnows now treat the player's short `PlayerSwimImpact` wake as a
nearby surface cue, alongside rain- and wind-made ripples. Within 2.5 m, the
school briefly widens its circling path for 1.2 seconds. A three-second
per-school cooldown prevents a single wake from repeatedly triggering a
response while the ripple is alive. The behavior is local, temporary, and
does not make a model request or change persistent world state.

The close-swimmer startle remains separate: a swimmer approaching the school
still causes the existing brief scatter-and-regroup response. The wake itself
is a gentler cue that broadens the school's orbit without scattering it.

## Regression coverage

`CaptiveSky2.Agent.NightEcology` now creates a nearby player wake and checks
that an untyped ripple and a distant swim wake are ignored, the school widens
its orbit after a nearby tagged wake, and the same wake does not retrigger during
the three-second cooldown. The test clears its transient cue and cooldown before
continuing the existing day/night checks.

## Validation status

The source and test files compiled and the isolated `CaptiveSky_2Editor`
development target linked successfully (UBT exit code 0; build log in
`%LOCALAPPDATA%/UnrealBuildTool/Log.txt`). A focused test launch is not yet
verified: the current UE 5.8.3 scratch project-open path previously crashed
during pre-initialization with the existing F405610C startup signature before
any automation test ran. See
[`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md).
This change must not be described as runtime-tested until the project-open
failure is resolved.
