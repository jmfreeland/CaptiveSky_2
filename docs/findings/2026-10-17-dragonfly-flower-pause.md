# Dragonfly bloom-pause readability

The existing Tideglass dragonfly-to-bloom cue now has an 8-second hover window
instead of 6 seconds, with the soft fade beginning at 6.8 seconds. The query
radius, weather/visitor/ripple eligibility, 1.2-second poll, and 16-second
per-dragonfly cooldown are unchanged; this lengthens the quiet observation
without causing more frequent visits or adding more wildlife.

The motivation is the bounded daylight review recorded in
`docs/worldbuilding-ideas.md`: dragonflies were active during a two-minute
real-RHI run, but none of its five sampled frames showed a bloom visit. That
does not prove the behavior is too rare, but gives a concrete legibility signal
for the next review.

## Validation

`CaptiveSky2.Agent.NightEcology` asserts that a bloom hover remains clearly
visible past 7 seconds, then fades and returns to patrol at 8 seconds. Both the
main target's compile-only build and the isolated UE 5.8.3 scratch target's
compile/link passed. The existing editor is still unresponsive and remains
untouched, so the automation assertion and normal-play visibility have not yet
been run; compare a bounded daylight Game capture against the earlier review.
