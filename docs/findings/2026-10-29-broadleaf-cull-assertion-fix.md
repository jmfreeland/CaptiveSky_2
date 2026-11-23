# Broadleaf cull regression check — 2026-10-29

The October 28 mid-distance trial intentionally gives Black Alder a 12,000 cm
fade start and 22,000 cm cull end; maple, aspen, and beech stay at 8,000/12,500
cm. The `BroadleafDiversity` automation still expected Alder to share the older
range, so its first live-editor run failed on those two assertions. The source
configuration was correct; the test was stale.

Updated the assertions to capture the intentional Alder range. Unreal Editor
Live Coding compiled and patched the test successfully. A repeat live-editor
run passed both `CaptiveSky2.Agent.IslandWeather.BroadleafDiversity` and
`CaptiveSky2.Agent.IslandWeather.BroadleafMixing`. The diversity check also
resolved the installed Black Alder skeletal mesh and verified its no-collision,
no-navigation settings. The mixing check verifies deterministic selection and
balanced four-species samples within groves.

After the approved editor restart, the live `r.Nanite.Foliage` tooltip read
`Current value: 1`. These checks prove the selection logic, asset resolution,
component safeguards, and configured cull distances—not the actual in-world
species distribution, whether the silhouettes read distinctly from the target
player viewpoints, or a performance/VRAM improvement. Those still need a
matched real-RHI Island capture with the existing 30-FPS floor and graphics
memory tracked; do not claim Nanite foliage is a performance win from the CVar
being enabled.

The first full live `CaptiveSky2.Visual.Viewpoints` run reported an error at
`02e_TideglassDragonflyClose`: that viewpoint depends on the optional daytime
dragonfly preview, which was not enabled for a normal capture. The harness still
saved its 11 other views, but marked the run failed. Marked the config entry as
day-dragonfly-only and taught the parser to skip it unless
`-ViewpointTideglassDragonflies` is enabled. Live Coding compiled the change;
the normal viewpoint test then passed and saved 11 captures to
`Saved/Viewpoints/2026-10-29_102430_h17.0`.

Visual review of shoreline, Tideglass ground-detail, Wind Arch, and resident-
scale views: ground cover is visible but reads in sparse, repeated clumps over
large bare areas at distance; the Island still reads predominantly as one
spruce silhouette. The day capture also shows placeholder white marker geometry
around the Listening Stones. These are visual findings, not proof of the
underlying population counts. This viewpoint test did not measure gameplay FPS
or VRAM, validate the player’s water collision, or test movement through the
current build. Do not mark those issues resolved from the capture pass.

Evidence-scope correction: the 11-shot pass was run without
`-ViewpointGroundCover`. The captures show the loaded editor-world scene, not the
transient procedural ground-cover preview. Their sparse/spruce-heavy appearance
does not establish that runtime broadleaf instances are missing. The broadleaf
unit tests exercise species-selection helpers and the Alder asset/cull settings;
they do not prove the live Island's generated tree counts or spatial coverage.

Tried the dedicated headless preview with `-GroundCover`, limited to the
Tideglass views and the daytime dragonfly preview. `Capture-Viewpoints.ps1`
returned before creating its requested log, and the `UnrealEditor-Cmd` process
surfaced a “The Game has crashed and will close” dialog. A direct `-version`
launch subsequently wrote the ordinary project log through clean `QUIT_EDITOR`
shutdown, but did not run the automation test. No runtime foliage, performance,
or VRAM result was obtained. Diagnose the commandlet startup path before another
headless preview attempt; do not use the base screenshots as a substitute.
