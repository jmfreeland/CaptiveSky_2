# Unreal Zen startup delay (2026-11-01)

The recurring "Zen Storage Server is slow to launch" notice is reproducible in the machine's Unreal 5.8 startup logs, but it is a slow cold start rather than a failed connection. The startup at 05:13:43 UTC found no Zen process using the data directory, launched the installed `zenserver.exe`, and reported the local HTTP service healthy at `[::1]:8558` at 05:14:32. Unreal recorded 48.978 seconds for Zen AutoLaunch initialization and then selected ZenLocal for DDC.

Recent archived Unreal logs recorded AutoLaunch durations of 37.659 seconds (03:00), 17.103 seconds (03:14), and 3.138 seconds (04:14). This variability matches the user's report that the warning recurs, while showing that it does not fail every time. In all inspected starts, the installed Zen version matched the engine's bundled version (`5.8.13-202605190912-windows-x64-release-094f62c2`). The Zen server log shows the prior server exiting because its sponsor processes had all exited, which is consistent with Unreal's documented default lifecycle: Zen starts and stops with the editor.

## Paired-launch follow-up

Reviewing the latest project logs changes the likely explanation for the repeated warning. On 2026-11-01 at 07:54:59 UTC, one editor launch started Zen against the shared `Common/Zen/Data` directory. A second editor launch began Zen initialization at 07:55:01 and logged that it was waiting for another process to finish startup/recovery. It timed out at 07:55:33, but immediately afterward its `ZenLocal` DDC reported `Status: OK`; the first launch also reported the HTTP service healthy at 07:55:51. The Zen server log confirms the service was listening on port 8558 from startup. A second pair of project logs around 06:19 shows the same wait/recovery pattern.

This is stronger evidence for overlapping editor startups contending on the shared Zen startup/recovery path than for a damaged or unavailable cache. The warning is still real and the delay is undesirable, but the observed launches recovered and attached to DDC. Avoid starting another editor copy while one is opening; do not clear Zen data based on this warning alone. The process list was unavailable during this inspection, so the log correlation does not identify which launcher started each overlapping editor.

Epic's [UE 5.8 Derived Data Cache documentation](https://dev.epicgames.com/documentation/unreal-engine/using-derived-data-cache-in-unreal-engine?lang=en-US) says Zen opens and closes with Unreal Editor by default and can be configured to remain running. That is a supported avenue to avoid repeated cold starts, but this inspection did not change service installation, system startup behavior, cache contents, or project configuration.

## Next diagnostic

If a single-editor launch still shows the warning, capture that launch's `Unreal.log` and `zenserver.log` with no second editor or Unreal automation starting concurrently, then compare the Zen process start/ready interval and any warnings around it. Consider keeping Zen alive between editor sessions only after confirming the preferred service/lifecycle setup; do not clear its cache as a first response. No restart or build is needed for this diagnosis.
