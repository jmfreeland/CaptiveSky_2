# Wildlife-home settlement through repeated use

Canonical wildlife-home records already held a shelter state and a last-used
timestamp, but resident rest only updated the timestamp. A temporary Rat
shelter therefore never became settled through lived use, and same-day repeated
sleep attempts had no distinct history.

The world-state record now stores the number of distinct Island days on which a
resident began resting at the home and the last such day. Repeated starts on the
same day are idempotent. After three distinct days, a non-disturbed home becomes
`Settled`; a disturbed home is never silently rehabilitated by this rule. The
existing sleep experience is still added only when the rest finishes, so waking
early records a home visit but does not claim a completed sleep. The fields are
optional on read, preserving existing version-2 world-state files.

`IslandNestTests` covers invalid-day rejection, same-day idempotency, the
three-day transition, JSON persistence, and preservation of an explicit
disturbance despite later use. The UE 5.8.3 editor target compiled and linked
successfully in the isolated `Claude_Props` scratch project (35 actions; 135.54
seconds). The focused automation test was not run: the project has a known
pre-test Core null-access startup failure. In-world review remains pending; this
change adds no model calls, map edits, or content assets.
