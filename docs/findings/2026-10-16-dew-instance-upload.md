# Dew instance upload optimization (2026-10-16)

`AIslandDewActor::Advance` previously called `UpdateInstanceTransform` for all
360 HISM slots on every frame, including frames where the viewer, active count,
and every placed glint were unchanged. This created avoidable render-state work
for a purely decorative effect.

The actor now retains which glints currently have visible instance transforms
and batches only changed slot indices. A transform is uploaded when a glint is
first seated, re-seated after leaving the viewer's 11 m disc, becomes active, or
is collapsed after becoming inactive. The existing deterministic scatter,
strength-driven count, material intensity, 48-seat-per-Advance cap, and
collisionless fixed 360-instance pool are unchanged. Once the scatter is seated
and the view is stationary, an `Advance` call uploads zero transforms; moving
the viewer at unchanged strength uploads only the newly seated batch.

`IslandDewRuntime` checks the initial seating bound, zero uploads after the
scatter settles, a nonzero upload after relocating the viewer, and that the
bounded seating batches eventually bring every active glint back inside the
disc. Static checks (`git diff --check`) pass, and the UE 5.8 editor target
compiled successfully from an isolated scratch project. Automation did not
start: the scratch NullRHI launch hit the known project-bootstrap crash before
tests began; details are in the [isolated validation finding](2026-10-16-scratch-ue-validation.md).
The main editor and its loaded module were left untouched. Runtime behavior
and frame-time improvement remain pending until the bootstrap failure is
diagnosed, followed by the focused test and matched dew-on/dew-off profile.
