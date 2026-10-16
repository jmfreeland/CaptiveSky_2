# View-local resident foliage sway focus — 2026-10-16

`AIslandWeather::UpdateGroundCoverSway` previously added every `ACharacter` in
the world to its resident-ground focus set, even when a resident had never been
visible to the active view. That created extra CPU transform work and
plant-bending neighborhoods which could not contribute to the current image.
The focus set now keeps player-controlled characters regardless of mesh
visibility and includes other characters only while their skeletal mesh was
recently rendered. Camera and active transient-gust focus points are unchanged;
nearby visible residents still bend the plants around their ground reference.
When a resident leaves view, the normal previous-index restore path returns
that resident's formerly swayed plants to their immutable base transforms.

The policy is isolated in `ShouldIncludeResidentInSwayFocus` and covered by the
`CaptiveSky2.Agent.IslandWeather` regression source: the controlled character
is retained, recently rendered wildlife is retained, and an offscreen
non-player is excluded. The opt-in console variable
`CaptiveSky.Island.FoliageSwayFocusTelemetry` (default `0`) reports candidate,
included, and excluded character counts alongside transform totals once per
sway pass.

The UE 5.8.3 editor target compiled and linked successfully in the existing
isolated scratch project (34 actions, including `IslandWeather.cpp` and
`IslandWeatherTests.cpp`). The automation test was not run, and no Unreal
Editor/Game process was started: the matching editor symbols remain absent,
and project-opening startup is still gated on symbolizing the retained Core
crash dump. No performance gain is claimed yet. After that gate is cleared,
repeat the same bounded 1600x900 Tideglass Game profile with the telemetry
enabled; compare included resident/transform counts and p50/p95 frame, Game,
Render, and RHI timings against the recorded full-focus and sway-disabled
baselines. Keep the local plant response and full species composition in the
visual acceptance review, along with the 30-FPS floor.
