# Minnow school follows Tideglass (2026-10-08)

The daytime minnow school's small vertical orbit now follows Tideglass's
lunar-driven waterline instead of remaining fixed to the pool marker's mean
level. Its surface-break ripple is placed against that same moving waterline.
The school remains local, visual-only, nonblocking wildlife; this adds no
agent decisions, model calls, or persistent state.

Validation: the isolated UE 5.8.3 editor target built successfully with the
user's editor open. `CaptiveSky2.Agent.TidepoolMinnows` passed in the scratch
project with agent thinking disabled and model requests capped at zero. The
regression compares the same fish at spring high and low water, checks that its
height above the water remains within its existing 10–24 cm swim band, and
checks that a splash follows the current tide offset.

This verifies the bounded transform behavior, not its visual readability from
normal play distance or the frequency that fish surface breaks are noticed.
