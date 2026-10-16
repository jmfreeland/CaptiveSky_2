# Periwinkle low-tide shelter posture — 2026-10-16

Tideglass periwinkles now use the Island's existing lunar tide to ease into a
small exposed-ebb shelter posture. As the water reaches the lowest part of its
cycle, each snail shortens its eyestalks, narrows the shell aperture, softens
its attention to close visitors, and reduces its already subtle idle motion.
As water returns, these cues extend smoothly back to their daytime baseline.

The response is transient and presentation-only. It does not move a snail's
home, change population or saved state, add collision, or create any Unreal
asset. Without an Island clock, the actor stays in its ordinary open posture.
`AIslandTidepoolPeriwinkle::ExposureRestAmount` bounds and rejects non-finite
tide inputs; the ecology test covers its extrema, monotonicity, and the visible
eyestalk/aperture response across an ebb/flood pair.

The UE 5.8.3 editor target compiled and linked successfully in the existing
isolated scratch project (7 actions; `IslandTidepoolPeriwinkle.cpp` and
`IslandEcologyTests.cpp` compiled). `CaptiveSky2.Agent.NightEcology` was not
run: Unreal commandlet startup still hits the retained unsymbolized Core
null-access crash documented in
[`2026-10-16-project-startup-minidump.md`](2026-10-16-project-startup-minidump.md).
No Unreal Editor/Game process was launched. The next validation is to run the
focused ecology test after the matching UE 5.8.3 editor symbols are available,
then inspect a bounded low-to-high tide capture at periwinkle height for a
readable but restrained posture change.
