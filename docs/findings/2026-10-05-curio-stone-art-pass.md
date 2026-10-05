# Curio stone art pass (2026-10-05)

Persistent pale trail stones and the resident-built cairn are now rendered with the same irregular
`SM_Rock` mesh and `M_Rock` material already used by the Wind Arch presentation. The saved curio
records, stable positions, instance counts, and one-stone-per-day contributions remain authoritative;
this changes only the transient renderer. `M_Rock` is loaded at runtime, so Claude's separate wetness
material work can still drive the shared asset without this change editing `Content/`.

If Starter Content is unavailable, curio stones keep their tinted Basic Shape material and sphere
mesh. The open Seedpod and its husks are unchanged. The test asserts the authored mesh/material when
available, the primitive fallback otherwise, and the cairn's collision/navigation invariants.

This is a source-and-automation change only until the focused UE 5.8.3 test passes and a Game/PIE
capture confirms the small cairn reads more naturally at resident viewing distance. The current
shared-target compile is blocked by separate in-flight `IslandArrangement.h` errors; do not treat
this visual improvement as runtime-validated yet.

The changed `IslandCurio.cpp` translation unit compiled to an object with the UE 5.8.3 target response
file and installed MSVC compiler. The changed automation translation unit could not be compiled
because its existing `IslandWorldStateSubsystem` include chain reaches the same unrelated
`IslandArrangement.h` errors. UHT, module linking, automation execution, and rendered inspection
remain outstanding.
