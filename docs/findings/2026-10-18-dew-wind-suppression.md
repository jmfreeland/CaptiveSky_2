# Morning dew responds to wind

`UIslandDewSubsystem` already samples rain, wetness, cloud cover, storm, sun
height and the Island clock, but omitted the live wind sample. As a result,
strong moving air could leave the ground glittering as if the morning were
still calm.

The dew calculation now consumes `UIslandEnvironmentSubsystem::GetWindSpeed()`
(centimetres per second) and uses the same 80–320 cm/s calm-air transition as
the Island mist calculation. Below 80 cm/s, wind does not reduce dew; a breeze
through that band progressively softens it; at 320 cm/s and above, dew is
suppressed. The existing twenty-second strength fade remains unchanged. No
resident schedule, model request, saved state, asset, or instance budget is
added.

The focused automation now checks calm, breezy, and clearing wind, and rejects a
non-finite wind sample. UE 5.8 `CaptiveSky_2Editor` compiled and linked these
changes successfully in the isolated `Codex_BasinRipple_20261018` scratch
project (9 build actions, 23.57 seconds). The automation executable was not
run: project-opening automation has repeatedly crashed before test startup with
the existing Core null-access hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`,
and the live editor is currently not responding to an activation request.
Therefore the new behavior is compile-verified, but not runtime- or visually
verified yet. The editor was left open because it reports one unsaved change.
