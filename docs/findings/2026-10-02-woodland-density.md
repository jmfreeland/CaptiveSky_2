# Expanding the Island woodland (2026-10-02)

The request for more vegetation is being addressed by expanding the deterministic spruce-grove
network from 400 to 500 centers. Each grove keeps its existing planting, spacing, landmark-clearance,
and bounded-trace rules; the 512-patch landscape scatter is unchanged. This is intended to add
forest islands and edge growth across the walkable landscape, not to fill the landmark clearings.
Nominal per-grove ceilings therefore rise by 25%: up to 26,000 trees (mature trees and saplings),
16,000 broadleaf shrubs, and 1,500 flowering rhododendrons. These are bounds, not predicted actual
counts; terrain acceptance and spacing determine the realized population.

The authoritative UE 5.8.3 baseline is the isolated RHI viewpoint run
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_Woodland400_RHI_20261002.log`:
it placed 11,965 spruce (7,148 saplings), 12,068 shrubs, and 1,191 rhododendrons across all 400
groves, after 43,257 bounded woodland traces. The same run placed 928,457 ground-cover instances
in the existing 512 meadow patches, and `CaptiveSky2.Visual.Viewpoints` passed. The isolated 120-second
Game run at the same 400-grove population shut down normally with zero model requests. Those results
are the comparison baseline only; they do not validate the new 500-grove source.

The 500-grove source change and corresponding population-bound assertions are pending a fresh UBT
build. To make safe progress while UBT is stalled, the changed
`IslandWeather.cpp` and `IslandViewpointCapture.cpp` translation units were compiled separately with
the scratch project's preserved MSVC 14.44 response files and engine `Source` working directory.
Both produced scratch-only object files and SARIF reports with empty `results` arrays:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/ManualObjectCompile_20261002/`.
This confirms translation-unit compilation only; the objects were not linked into the editor module.

On 2026-10-03, the connected UE 5.8.3 editor ran `CaptiveSky2.Visual.Viewpoints` with a real RHI and
passed in 17.05 seconds with no test errors, saving all nine configured frames to
`Saved/Viewpoints/2026-10-03_090731_h17.0/`. A later audit found the editor command line did not
include `-ViewpointGroundCover`; that flag gates the real-Island ground-cover initialization and the
500-grove population/spacing assertions. Therefore this run verifies the map/camera/render capture
path only. Its images do not prove the 500-grove source was loaded, nor do they validate vegetation
placement or packaged performance. The ground-cover-enabled capture and a fresh UBT build remain
pending.

The same live editor also passed `CaptiveSky2.Agent.GroundCover` and
`CaptiveSky2.Agent.NightEcology` (0.41 seconds total, no errors or warnings). These are useful
fixture/regression checks, but they do not populate the real Island with the 500-grove scatter.
`Scripts/Capture-Viewpoints.ps1 -GroundCover` is the supported real-Island RHI route; its documented
workflow requires the project editor to be closed. It has not been run while the user's editor is
open. The in-editor Live Coding compile tool also reported that Live Coding is disabled for this
session.

On 2026-10-02 the isolated `Build.bat CaptiveSky_2Editor Win64 Development`
invocation with `-WaitMutex` emitted only its launch line and no UBT log or build artifacts for over
three minutes. Only that launcher was cancelled; the open editor and other processes were left alone.
Follow-up read-only inspection found two older `dotnet` processes (created September 29 and October 1),
but Windows returned no executable path, command line, or inspectable parent for either, even with an
elevated query. Their relationship to UBT is unknown; neither was stopped. The installed UBT's stall
point remains unidentified (see the build-triage note in
`2026-09-28-long-runs.md`); do not bypass its mutex or stop any process whose owner cannot be
verified. Resume compilation and the `CaptiveSky2.Visual.Viewpoints` test when that condition clears.

No ignored `Content/` assets, map files, agent memory, or live world state were changed.
