# Report clock and environment snapshot failures

`AIslandDayNight::PersistHour` previously discarded the boolean results from
the durable clock and wetness writes. It now returns success only when the
required writes succeed, logs periodic and final `EndPlay` failures, and lets
the focused persistence fixture assert a successful explicit boundary flush.
The authored `bResumeSavedTime=false` mode still intentionally skips saving the
clock; environment wetness remains persisted when available. When both clock
resume and environment state are active, hour/day/wetness now go through one
atomic world-state replacement instead of two consecutive replacements, so a
failure cannot leave a new clock paired with old wetness.

The fixture also supplies an unreadable existing world-state file and asserts
that the explicit flush reports failure and leaves the original payload
untouched. A separate missing-parent write fixture forces the atomic file
replacement to fail and asserts that neither the in-memory clock nor wetness
is committed. These exercise the fail-closed contract without weakening the
subsystem's refusal to overwrite unreadable user data.

The isolated UE 5.8.3 `CaptiveSky_2Editor` target built successfully after the
change. The `CaptiveSky2.Agent.DayNightPersistence` automation source also
compiled, including successful persistence/reload and unreadable-file and
failed-write cases, but was not run: the known Unreal
commandlet pre-initialization crash has not been diagnosed. This is not proof
that a live shutdown flush succeeds under filesystem failure.

This is a prerequisite for the resolved hosted empty-world policy, not its
implementation. A future authoritative visitor-lifecycle coordinator must
check this result before claiming a clean pause, reject new visitors if the
flush fails, and fence late resident callbacks at the session boundary. No
visitor accounting, network authentication, or host shutdown behavior was
added here.
