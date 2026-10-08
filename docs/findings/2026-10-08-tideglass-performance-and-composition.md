# Tideglass fixed-view Game checkpoint (2026-10-08)

A standalone UE 5.8.3 Game sample loaded the Island and held the existing
Tideglass motion-probe composition. Agent thinking and Python were disabled,
the data root was isolated under `Saved/Playtests/Codex_TideglassCurrent_20261008`,
and the run ended itself at its 120-second real-time cap with zero model
requests. Startup required a warmed-cache retry: the first attempt hit its
300-second startup watchdog during first-run engine material/texture shader
compilation and DDC work; the retry reached the Island in 89.7 seconds. No
interactive editor process was used or changed.

The CSV profiler captured 600 frames over 10.24 seconds at 1600x900. Frame time
was 17.17 ms mean, 14.80 ms p50, 24.83 ms p95 (about 40.3 FPS), and 171.78 ms
maximum. This stationary, fixed-view sample clears the 30-FPS p95 floor, but
the maximum exposes a hitch tail. It is one short sample, not proof that PIE,
travel, weather transitions, or target hardware meet the same threshold.

## Hitch-tail drilldown

The same 600 numeric frames in [`Profile(20261008_010737).csv`](../../Saved/Profiling/CSV/Profile%2820261008_010737%29.csv)
contain 19 frames over 33.33 ms and nine over 50 ms. Sixteen frames exceed
33.33 ms on the Render Thread, compared with three on the Game Thread and one
on the GPU. In the 171.78 ms maximum-frame row, GameThreadTime is 20.51 ms,
RenderThreadTime is 139.85 ms, GPUTime is 23.05 ms, and RHIThreadTime is 9.73
ms. The largest exclusive Render Thread samples in that row are EventWait
(50.23 ms), RDG (39.81 ms), and EventWait/Visibility (15.70 ms); the Game
Thread also spends 117.73 ms in EventWait. These overlapping thread samples
are not additive, and a single CSV row does not establish the underlying wait
owner or root cause.

This points toward variable render-thread synchronization/RDG work in this
capture, not a demonstrated GPU-bound frame or a proven ground-cover-density
cause. Do not tune foliage density or culling based on the maximum frame alone.
The longer [controlled Insights record](2026-10-04-actual-gameplay-profile.md)
already compares Lumen, CPU ground-cover sway, occlusion queries, and the RHI
thread. It linked some earlier ~0.45–0.48 second stalls to CPU instance updates
and task/sync-point waits, but this shorter 10/8 hitch is not proven to share
those causes. Avoid repeating those broad ablations. If the hitch recurs after
the current foliage changes settle, capture an event-level trace of that exact
default config with screenshots disabled and correlate Render Thread EventWait
/ RDG against `GameThreadWaitForTask`; separately keep the bounded moving-PIE
check distinct from this fixed-view profile. This does not change the p95
result or the separate need to improve the broad view's ecological composition.

The [Game screenshot](../../Playtests/Codex_TideglassCurrent_20261008/Screenshots/003_Tideglass.png)
shows a reflective pool, a dense and partly tangled foreground, broad sparse
brown middle distance, and thin distant tree silhouettes. The camera makes the
landscape's structural issue clear: coverage is not yet organized into an
intentional wet edge, open circulation, meadow masses, and woodland transition.
This is a Game render, unlike an editor-world preview, but it is not a species
or materials beauty pass.

## Next checkpoint

Keep the matched 1600x900 camera and 600-frame capture as a comparison while
the ecological-band work evolves. Retain separate results for the static broad
view and ordinary moving/PIE play; do not infer gameplay performance from this
fixed camera. Art-direct one narrow wet-edge band and open circulation lane,
then recheck both the composition and the p95 target before generalizing zones.

Log: [`SpectatorStartup_20261008_010524_634.log`](../../Saved/Logs/SpectatorStartup_20261008_010524_634.log).

## Solo-layer composition captures

The updated `CaptiveSky2.Visual.Viewpoints` harness was exercised in a no-world-
state editor-world capture at 11:00 with transient Tideglass dragonflies and the
temporary `M_TideglassPool_Lively` water preview. It passed and reported
1,780,640 nonblocking ground-cover instances. The `meadow` layer capture
measured 49.09 FPS p95 SceneCapture throughput; the separate `woodland` layer
capture measured 52.04 FPS p95. These offscreen layer diagnostics are not Game
or PIE performance measurements.

The [meadow-only frame](../../Saved/Viewpoints/2026-10-08_013038_h11.0/02_Tideglass.png)
shows continuous, tangled ground cover encroaching on the pool with no clear
circulation lane. The [woodland frame](../../Saved/Viewpoints/2026-10-08_013218_h11.0/02_Tideglass.png)
shows a sparse canopy over a broad bare brown midground. Together, these support
the existing diagnosis: the next composition should be a deliberately
intermediate habitat—an open lane and narrow wet edge in the foreground, grouped
meadow masses through the middle, and a denser but readable woodland edge—rather
than globally adding more instances. The temporary water material reads cyan and
the Wind Arch proxies remain blockout shapes; these captures do not validate the
current Game water material or final landmark art.

A third no-world-state capture isolated only the `groundplants` layer, using the
now-validated `-TideglassDragonflies` wrapper option. The viewpoint automation
passed at 47.24 FPS p95 SceneCapture throughput; this remains an offscreen
diagnostic, not Game or PIE performance evidence. The [ground-plants-only
frame](../../Saved/Viewpoints/2026-10-08_014251_h11.0/02_Tideglass.png) still
reads as nearly continuous foreground cover, with no clear circulation lane.
That shows the issue is spatial organization, not merely species choice: the
next production pass needs to reserve an open corridor and confine wet-edge
plants to a narrow band rather than scattering them uniformly across the view.
The transient pool material remains a preview only.

The initial wrapper attempt failed its dragonfly close-up because the script did
not forward the harness's daytime-dragonfly preview flag. `Capture-Viewpoints.ps1`
now exposes `-TideglassDragonflies`, validates daylight and its compatibility
with the night-firefly preview, and the full meadow-layer capture passed through
the wrapper at 46.71 FPS p95 SceneCapture throughput. An hour-20 negative check
was rejected before launch. Log: [`Codex_TideglassMeadowWrapper_20261008.log`](../../Saved/Logs/Codex_TideglassMeadowWrapper_20261008.log).
No saved map or world state was changed and no model request was made.

## Shared wetland layer checkpoint

The current working-tree foliage preview was exercised without changing its
source or map. The UE 5.8.3 target was already up to date; headless
`CaptiveSky2.Agent.IslandWeather` passed, and the no-world-state
`CaptiveSky2.Visual.Viewpoints` meadow-layer capture passed with transient
dragonflies. It reported 7 Typha plus 3 Phalaris, 559.6 cm minimum pool-edge
clearance, and 440.8 cm minimum shared plant spacing. The `meadow` diagnostic
kept 19 HISM components visible. SceneCapture p95 throughput was 50.17 FPS at
the overview, 47.51 FPS at ground detail, and 51.16 FPS for the close
dragonfly frame. These are offscreen editor captures only; they do not verify
Game/PIE performance or the authored landscape/water materials.

The [current meadow frame](../../Saved/Viewpoints/2026-10-08_035323_h11.0/02_Tideglass.png)
still reads as dense foreground cover over a broad sparse brown midground; the
white pool and blocky landmark shapes are editor-world placeholders, not final
Game art. The layer isolation makes the next art direction clearer: reserve a
legible circulation opening and group a visibly distinct wet-edge band, rather
than increasing total meadow coverage. Logs: [IslandWeather regression](../../Saved/Logs/Codex_SharedIslandWeatherValidation_20261008.log)
and [Viewpoints capture](../../Saved/Logs/Codex_SharedWetEdge_Meadow_Dragonflies_20261008.log).

## Live Simulate observation

After the local Unreal MCP bridge was restored, the actual Island ran in
Simulate-In-Editor for about 83 seconds. The editor launch used a separate
world-data root, disabled resident thinking and set the model-request cap to
zero; the session was stopped manually and confirmed idle afterward. A live
viewport capture at the Tideglass composition showed runtime ground cover,
the stag, and the raven rather than the offscreen harness's transient proxy
preview. The ground cover still crowds the foreground and does not form a
clear circulation lane; the pool surface reads pale and flat in this Simulate
view, so verify it again in Game before judging the authored runtime material.

The editor log reports a 306 cm pool-edge clearance and “10 nonblocking
cattails,” while the passing wetland composition automation reports 7 Typha
plus 3 Phalaris. Treat the count as consistent but the runtime species label
as stale instrumentation; the shared `IslandWeather` work was not edited.
This was a brief visual observation, not an FPS/profile measurement or a
30-FPS gameplay claim. Log: [bounded MCP editor session](../../Saved/Logs/Codex_EditorMcpRecovery_20261008.log).
