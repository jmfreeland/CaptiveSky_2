# Periwinkle wet-shell response

Tidepool periwinkles now darken their natural shell and spiral palette gradually
when submerged by high tide or exposed to heavy rain. The response is transient:
it is derived from the live tide/rain samples, is not serialized, and has no
effect on placement, collision, requests, or resident memory. The shell tint
eases toward the current environment rather than switching abruptly; material
updates are skipped when the change is visually negligible.

`WetShellAmount` is a pure, bounded mapping. High tide fully wets the shell,
exposed ebb dries it, and heavy rain contributes a smaller wetness value. A
runtime ecology assertion also compares dry and wet dynamic-material colors.

## Validation

The change is covered by `CaptiveSky2.Agent.NightEcology` assertions for tide,
rain, non-finite values, and the palette response. The user's Unreal Editor is
currently open and unresponsive; by request it remains running, so the main
project module cannot be relinked while its DLL is loaded. The source and tests
have not yet been compiled or run in the editor during this iteration. Do not
interpret the source-level checks as runtime validation.
