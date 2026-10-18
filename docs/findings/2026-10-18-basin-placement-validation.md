# Rain basin saved-placement validation — 2026-10-18

The rain-basin save reader now validates all three persisted location values as
JSON numbers and checks that each remains finite after conversion to Unreal's
float-backed `FVector`. If a coordinate is missing, has the wrong JSON type,
or overflows the transform range, the basin's `placed` flag is cleared while
other valid basin state (such as water and leaves) continues to load. This
prevents corrupt or malformed save data from materializing the basin at an
invalid world transform without discarding the recoverable state.

The focused automation adds malformed-string and float-overflow coordinates,
and checks that water survives both cases. During the UE 5.8.3 editor-target
build, `IslandRainBasin.cpp` and `IslandRainBasinTests.cpp` both compiled. The
final DLL link failed because Windows reported the output DLL was held by PID
22688. An elevated read-only process check confirmed that PID is this
project's `UnrealEditor.exe` (started 2026-10-16 10:04:33, main window
`CaptiveSky_2 - Unreal Editor`, `Responding=False`). The failed process query in
the normal sandbox had missed it. Per the user's earlier direction to leave
this editor running, it was not closed or terminated.

To get an independent link, the same source and test changes were mirrored into
the source-only scratch project
`Saved/CompileScratch/Codex_MegaplantCull_20261018` (separate `Binaries`, no
Island `Content`). Its editor target compiled both basin units and linked
successfully in five actions. A focused NullRHI run used agent thinking disabled
and a zero model-request cap, but crashed at `StartOfEnginePreInit` before
automation began: null-address access violation at `SecondsSinceStart=0`, with
the already known stack hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The crash-side log is
`Saved/CompileScratch/Codex_MegaplantCull_20261018/Saved/Crashes/_0000/CaptiveSky_2.log`.
This reproduces the project-opening crash without Island Content and is not a
test result; do not repeat the same launch until its startup environment or
configuration meaningfully changes.

The main-tree target still did not relink because PID 22688 holds its DLL. No
Island map, Content asset, or persistent world-state file was changed.
