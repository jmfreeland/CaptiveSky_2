# Listening Stones: non-interruption etiquette

An approaching player or autonomous resident no longer triggers a visitor
resonance while a different autonomous resident is holding the close quiet-visit
latch. The arriving visitor is latched for the existing rearm distance, so the
landmark does not retry a chime on every tick; leaving and returning allows a
new approach response. The quiet listener's own eight-second cue is unchanged.

This is intentionally a small courtesy cue, not path planning: visitors are not
automatically moved to an edge position. It adds no model request, memory write,
or canonical world-state mutation. Deterministic and synthetic-world assertions
were added. The UE 5.8.3 isolated editor target compiled and linked successfully
on 2026-10-29. A headless automation launch did not produce a fresh project/test
log, so no runtime pass is claimed; the open editor was left untouched and no
PIE session was started.
