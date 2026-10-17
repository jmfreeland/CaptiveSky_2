# Rat–raven quiet recognition

An idle Rat can now notice an actual Raven nearby when it has an unobstructed
visibility trace. It briefly turns toward the bird and uses its existing
Fox-derived idle-look animation. The cue is limited to 8 m, lasts about 1.7
seconds, and has a 16-second cooldown. It does not move either character, speak,
write memory or world state, interrupt an existing route, or request a model.
Resting residents are left undisturbed; the Raven must also have a visible body
and not be resting.

`CaptiveSky2.Agent.RatRavenAttention` covers the range, visibility, movement,
rest, cooldown, and invalid-distance gates. The UE 5.8.3 editor target compiled
in the isolated scratch project. The focused automation test has not been run:
the project's known Core null-access startup crash occurs before the test queue
starts. In-world timing and readability still need a bounded zero-request
review after the editor startup issue is resolved.
