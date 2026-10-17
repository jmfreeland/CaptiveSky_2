# Storm-wrack crab response (2026-10-17)

Turning over storm-fallen driftwood can reveal signs of a small crab sheltering
under the wood. The discovery is worded as a hollow and damp tracks, so the
description stays true even if the crab has already moved on. The wrack
interaction now looks for an actual shore crab within
175 cm only when that deterministic find is selected. An unsheltered crab
that is not already scurrying briefly darts sideways; distant wildlife and all
other finds remain unaffected. This is transient movement only: no population,
ledger, chronicle, resident memory, world-state, or model-request behavior is
added.

`CaptiveSky2.Agent.IslandWrackCrabDisturbance` covers the matching nearby case,
a crab outside the shelter radius, a sheltered crab, and a deterministic
driftwood find with no crab. It also checks a crab-trace discovery when no
matching actor is present, ensuring the narration does not claim a scurry.
It uses an isolated transient world and temporary wrack/chronicle files.

The clue text now reports a crab hollow and damp tracks; a nearby live actor
adds the brief sideways scurry only when one is actually present. This keeps a
valid, persistent trace distinct from a transient movement event.

**Validation boundary:** `git diff --check` reported no whitespace errors.
The UE 5.8.3 build ran UHT and compiled the current `IslandWrack.cpp` and
`IslandWrackTests.cpp`; the incremental retry after adding the sheltered-crab
assertion recompiled the test unit and linked the module library. Final DLL
linking failed because the project's `UnrealEditor-CaptiveSky_2.dll` was locked
by `UnrealEditor.exe`; the non-UBA link retry failed on the same lock. The
editor was left untouched, and the focused automation has not run. Run
`CaptiveSky2.Agent.IslandWrackCrabDisturbance` via the isolated target below
once project startup is reliable; do not treat this as runtime-verified yet.

**Isolated retry (2026-10-17):** a separate scratch project was populated from
the current source tree and built into its own `Binaries/Win64` directory, so
the editor's project DLL was not relinked. The UE 5.8.3 editor target completed
successfully and a subsequent UBT check reported the target up to date. The
focused command-line automation did not reach the test: Unreal crashed during
startup with a Core-only null-address access violation before project
initialization, matching the 2026-10-13 startup signature
`F405610C114D120D4B6F5ABA901EC469AC9FAA26` (see the separate bootstrap
diagnosis in `worldbuilding-ideas.md`). The first launch used an empty `Content`
directory; I then attached the full main-project Content tree through a junction
and retried. The second launch crashed with the same stack hash before
initialization, so the empty directory was not the immediate cause. This is not
a test assertion failure; the behavior remains unverified until the project
startup crash is resolved.

**Truthful clue follow-up (2026-10-17):** the isolated UE 5.8.3 editor target
rebuilt successfully after the clue wording and absent-crab regression were
added (5 actions, including `IslandWrack.cpp` and `IslandWrackTests.cpp`). The
automation still has not run; the pre-test Core startup failure above remains
the validation boundary.
