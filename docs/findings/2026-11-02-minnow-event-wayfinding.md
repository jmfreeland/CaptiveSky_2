# Minnow event wayfinding

Residents' next already-scheduled action context now describes the actual
location of a recent minnow surface break by approximate horizontal distance
and relative direction. The location is captured from the same transient
ripple created for the brief hop, so the cue points to where the surface event
occurred rather than where the school has since moved. The splash duration,
five-minute observation window, and wildlife behavior are unchanged. The cue
remains read-only context: it does not create a movement/interact target, force
an inspection, add a memory, or issue another model call.

The focused `IslandMinnowTests.cpp` regression now positions its observer
relative to the recorded event and asserts an “about 3 metres ahead” cue. The
isolated UE 5.8.3 Game target compiled successfully with the updated controller,
minnow, and regression sources. The known zero-second Unreal startup crash
prevents running the automation at this time, so the resident behavior remains
compile-checked rather than runtime-validated.
