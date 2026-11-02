# Visitor minnow event observation

The visitor's existing on-demand `E` nature-observation cycle now includes a
recent minnow surface-break cue when the visitor is within the same existing
11 m habitat radius and height window used for nearby resident context. It
reuses the transient event location and relative direction/distance formatter;
the ripple still fades in under a second, and the observation remains
available only for its existing short context window. The event does not
become an interaction or movement target. No extra per-frame work, scheduled
scan, model call, memory write, or world mutation was added.

The focused `CaptiveSky2.Agent.VisitorWildlifeObservation` regression now
checks that a visitor can receive the “about 3 metres ahead” event caption
through the existing E cycle. The isolated UE 5.8.3 Game target compiled and
linked successfully. Runtime automation remains unverified because the current
project startup still hits the repeated zero-second Core access violation.
