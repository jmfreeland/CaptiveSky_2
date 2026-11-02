# Visitor plant wayfinding

The on-demand `E: notice your surroundings` plant observation now includes the
nearest visible patch's approximate distance in metres and its dominant
relative direction (ahead, behind, left, or right). The location comes from the
same already-selected visible instance used to describe the plant; this adds no
instance scan, trace, model call, persistent write, or landscape change. If the
caption has no usable location, it retains the generic “nearby” wording. The
observation continues to say that every plant remains growing, rooted, and
undisturbed.

The isolated UE 5.8.3 Game target compiled successfully, including the focused
vegetation test source. The test asserts a 3 m forward cue and a safe fallback
for an unlocated patch. The project still fails during zero-second Unreal
startup with the previously recorded Core access violation, so the automation
test and rendered caption have not been observed at runtime. Treat this as
compile-checked, not live-validated.
