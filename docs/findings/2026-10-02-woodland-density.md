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

The 500-grove source change and corresponding population-bound assertions were pending a fresh UBT
build. During the earlier UBT startup stall, the changed
`IslandWeather.cpp` and `IslandViewpointCapture.cpp` translation units were compiled separately with
the scratch project's preserved MSVC 14.44 response files and engine `Source` working directory.
Both produced scratch-only object files and SARIF reports with empty `results` arrays:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/ManualObjectCompile_20261002/`.
This confirms translation-unit compilation only; the objects were not linked into the editor module.

On 2026-10-03, the connected editor ran `CaptiveSky2.Visual.Viewpoints` with a real RHI and passed,
but its command line omitted `-ViewpointGroundCover`, which gates real-Island scatter initialization
and the 500-grove assertions. That first nine-frame capture verifies only the render/camera path; it
does not validate woodland placement.

The follow-up used the isolated scratch project, whose `IslandWeather.cpp` and
`IslandViewpointCapture.cpp` hashes matched the committed source. An elevated UE 5.8.3
`Build.bat CaptiveSky_2Editor Win64 Development -WaitMutex` build completed successfully in 31.05
seconds (5 actions): both changed translation units compiled, the project module linked, and the
target metadata was written under
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/`.

That scratch editor then ran `CaptiveSky2.Visual.Viewpoints` with a real D3D12 RHI,
`-ViewpointGroundCover`, `-ViewpointNoWorldState`, and the Wind Arch view at 12:00. The test passed
with no automation errors and saved
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-03_092617_h12.0/04_WindArchOverlook.png`.
The actual Island scatter reported 14,727 trees (8,793 saplings), 15,063 broadleaf shrubs, and 1,487
flowering rhododendrons in 500 groves after 54,474 bounded woodland traces. The existing 512 meadow
patches contributed 928,457 instances. Automated spacing/clearance, collision, and navigation checks
passed; this is a real-RHI placement/render validation, not packaged-performance evidence.

The same live editor also passed `CaptiveSky2.Agent.GroundCover` and
`CaptiveSky2.Agent.NightEcology` (0.41 seconds total, no errors or warnings). These are useful
fixture/regression checks, but they do not populate the real Island with the 500-grove scatter.
The isolated build/capture was run while the main editor remained open; output, logs, and DDC were
confined to the scratch project's `Saved/` and `Intermediate/` directories. Its `Content/` junction
was read-only for this test and no assets or world state were saved.

On 2026-10-02 the isolated `Build.bat CaptiveSky_2Editor Win64 Development`
invocation with `-WaitMutex` emitted only its launch line and no UBT log or build artifacts for over
three minutes. Only that launcher was cancelled; the open editor and other processes were left alone.
Follow-up read-only inspection found two older `dotnet` processes (created September 29 and October 1),
but Windows returned no executable path, command line, or inspectable parent for either, even with an
elevated query. A user-authorized stop attempt was denied by Windows. Both processes remained alive
through the successful isolated UBT build and RHI test, so they were not a blocker for this route; their
purpose and relationship to other build attempts remain unknown. Do not bypass the build mutex.

No ignored `Content/` assets, map files, agent memory, or live world state were changed.
