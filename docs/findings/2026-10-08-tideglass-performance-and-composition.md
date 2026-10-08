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
