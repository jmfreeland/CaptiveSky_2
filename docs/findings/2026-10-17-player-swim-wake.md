# Player surface-swim wake — 2026-10-17

## Change

The player now leaves a short procedural ring wake while surface swimming, but
only while moving horizontally at 80 cm/s or faster. A successful wake spawn is
rate-limited to one every 1.15 seconds, placed 48 cm behind the swimming
direction at the queried ocean surface. It uses a 1.1-second, 82 cm ring with
a restrained aqua emissive tint and zero point-light intensity. It is tagged
`PlayerSwimImpact` and does not call the Tideglass material-ripple subsystem,
so this open-ocean player cue stays separate from the prototype pool and
its material ripple. The minnow school has a separate ecological response to a
nearby surface-swimming player: it briefly scatters within its 5 m startle
radius, observes an 8-second cooldown, and rearms after the player leaves. The
wake itself makes no LLM or network request.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` scratch build completed successfully
with 173/173 actions, including `CaptiveSky_2Character.cpp`,
`IslandPoolRippleEffect.cpp`, and `IslandOceanTests.cpp`. Follow-up incremental
builds also succeeded after the player-only tint change and after adding a test
assertion that the visible wake ring has a material.

The focused NullRHI automation launch did not reach the test. Two launches of
the scratch project ended at `SecondsSinceStart=0` with a null-address access
violation in `UnrealEditor-Core`; the second launch used scratch-relative DDC,
shader-work, and log paths to rule out the spaced absolute paths as the cause.
Both recorded stack hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`, matching the
previously documented unsymbolized UE commandlet startup crash. The fresh dump
is at `Saved/CompileScratch/Codex_PlayerSwimWake_20261017/Saved/Crashes/_0000/`.
There are no matching editor PDBs in the installed UE 5.8.3 `Win64` binaries,
so this evidence cannot identify the root cause. The wake assertions compiled
but have not run, and the wake has not had a visual in-game review. The adjacent
minnow behavior is covered by the existing
`CaptiveSky2.Agent.TidepoolMinnows` test, but that test has not been rerun in
the current startup environment.

The user's existing Unreal editor was left untouched. Do not repeat this
commandlet launch until the startup stack can be symbolized or a meaningfully
different, safe editor-driven validation route is available. Next: resolve the
matching engine dump, then run `CaptiveSky2.Agent.IslandOcean` and visually
check the wake in open-water play before treating the interaction as verified
or shipping it.
