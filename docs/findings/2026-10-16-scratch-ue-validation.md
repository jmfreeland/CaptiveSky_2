# Isolated Unreal validation (2026-10-16)

To avoid replacing the DLL loaded by the open Island editor, a disposable
project shell was created under
`Saved/CodexValidation_DewMinnow_20261016/`. It contains copies of the project
descriptor, `Source/`, and `Config/`, plus the single optional dew material
asset; generated binaries, intermediates and logs stay inside that ignored
folder. The main project's loaded module and map were not modified.

The UE 5.8 editor target compiled successfully from this copy: 173/173 actions,
including `IslandDew.cpp`, `IslandDewTests.cpp`, `IslandTidepoolMinnows.cpp`,
and `IslandMinnowTests.cpp`. UBT reported `Result: Succeeded` and 665.70 seconds
total execution time.

A subsequent NullRHI commandlet attempt for
`CaptiveSky2.Agent.IslandDewRuntime` and `CaptiveSky2.Agent.TidepoolMinnows`
crashed before the project log/test queue began. CrashContext reports a null
access violation at `SecondsSinceStart=0`, with
`PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`; this matches the
previously recorded project-open bootstrap failure. The request reached the
Unreal process, but neither automation test ran. Do not repeat the same launch
until the bootstrap failure is diagnosed or the environment meaningfully
changes.

Next: diagnose this shared startup path using the existing crash reports, then
rerun the two focused tests from the scratch project. Only after they pass,
profile the dew effect and review minnow motion in a bounded real-RHI Game view.
