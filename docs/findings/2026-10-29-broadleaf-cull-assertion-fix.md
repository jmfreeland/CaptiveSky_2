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

Attempted the full live `CaptiveSky2.Visual.Viewpoints` capture after the
automation fix. It stopped at `02e_TideglassDragonflyClose` because no actor is
tagged `TideglassDragonfly` in the loaded Island. It therefore produced no
usable viewpoint or population/performance validation. Keep the existing
viewpoint config and Island map untouched until the missing target is resolved
or that capture is deliberately redesigned.
