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

The first wrapper invocation failed only because it omitted the harness's
daytime-dragonfly preview flag; the corrected UE 5.8.3 invocation passed. The
current wrapper does not yet expose that flag, so the successful layer captures
used direct command-line arguments. No saved map or world state was changed and
no model request was made.
