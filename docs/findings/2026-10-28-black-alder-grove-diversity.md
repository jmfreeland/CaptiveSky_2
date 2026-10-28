# Black Alder grove diversity — 2026-10-28

## Change

`AIslandWeather` now loads the local Megaplant Black Alder A skeletal mesh and
places it as a fourth deterministic, grove-clustered broadleaf species beside
Norway maple, European aspen, and European beech. Grove field values are mapped
evenly into four species bands; instance rotation, target height, collision,
wind-provider setup, and the existing 80 m / 125 m cull range follow the same
path as the other broadleaf trees.

The broadleaf population remains capped at 3,600 instances and replaces spruce
placements rather than adding trees. When all four meshes resolve, each species
has a 900-instance ceiling. If one or more local assets are absent, the ceiling
is divided among the meshes that did load, so the mixed-tree allocation can
still reach its prior share of the fixed population. If none resolve, all tree
placements fall back to spruce as before.

## Verification

- The actual main UE 5.8 Editor target built and linked successfully (16 UBT
  actions), compiling the current working-tree changes and the new test.
- `CaptiveSky2.Agent.IslandWeather.BroadleafDiversity` then passed in the live
  editor at 2026-10-28 21:49:20. It covers four-way field mapping,
  endpoint/invalid-field bounds, fair-share budgets for one through four
  available species, Black Alder component settings, and the local mesh path.
  Evidence: `Saved/Logs/CaptiveSky_2.log` and the Editor Message Log.
- A separate `UnrealEditor-Cmd -NullRHI` attempt crashed at process startup
  before test discovery with the known null-read signature
  `F405610C114D120D4B6F5ABA901EC469AC9FAA26`; the in-editor run above is the
  successful behavior-test evidence.
- The main editor is open on the Island with the new module loaded and reports
  `All Saved`; no map or asset was saved. Live mesh assignment, Nanite
  appearance, wind response, grove distribution, and frame-time/VRAM cost are
  still unverified. The existing in-flight `IslandViewpointCapture.cpp`
  diagnostic enumerates only the three earlier broadleaf components and was
  deliberately left untouched.

## Next validation gate

When the active capture-test work is clear and a controlled editor validation
window is available, include Black Alder in the woodland-layer diagnostics,
confirm the live log reports four species with no increase in total trees,
compare matched mid-distance Island views, and measure frame time and VRAM
before treating the new species as visually and operationally approved.
