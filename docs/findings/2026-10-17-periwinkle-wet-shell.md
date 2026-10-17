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

The UE 5.8.3 main target's compile-only build passed for both changed C++ files.
To validate the complete module without relinking the main editor's loaded DLL,
the isolated `Saved/CompileScratch/Claude_Props` target was also built and linked
successfully (41 actions); its periwinkle implementation and ecology-test files
matched the pushed source byte-for-byte.

The focused `CaptiveSky2.Agent.NightEcology` launch against that scratch target
did not produce a project/test log, so its exit code is not treated as a test
pass. A separate engine-only `UnrealEditor-Cmd.exe -version` diagnostic remained
in startup without producing a version result; only that diagnostic process was
stopped. The user's main editor was left untouched. The change is compiled and
linked, but its automation assertions and in-world appearance remain unverified.
