# Rain-basin leaf-setting response

When a resident successfully sets a leaf afloat, the basin now starts a short transient ripple centered on the current water surface. Rain impacts use the same generic `StartWaterRipple` entry point. This gives a lasting resident action an immediate visual response without changing the basin's save format or making the effect persistent. A too-dry or same-day refused interaction still does not start or restart the ripple.

Validation on 2026-10-18:

- The UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked successfully in an isolated scratch project; both `IslandRainBasin.cpp` and `IslandRainBasinTests.cpp` compiled as part of 174 build actions. The live editor's project DLL and unsaved map state were not rebuilt over.
- The focused `CaptiveSky2.Agent.IslandRainBasinWorld` automation did not reach test discovery. `UnrealEditor-Cmd` crashed during `StartOfEnginePreInit` with a null-read access violation and the existing Core stack hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`, even with no `Content/` directory attached to the scratch project. No behavior-test pass is claimed.
- The editor remains open with one unsaved change and was not closed or otherwise altered. In-world visual verification remains pending.
