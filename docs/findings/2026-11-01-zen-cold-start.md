# Unreal Zen startup delay (2026-11-01)

The recurring "Zen Storage Server is slow to launch" notice is reproducible in the machine's Unreal 5.8 startup logs, but it is a slow cold start rather than a failed connection. The startup at 05:13:43 UTC found no Zen process using the data directory, launched the installed `zenserver.exe`, and reported the local HTTP service healthy at `[::1]:8558` at 05:14:32. Unreal recorded 48.978 seconds for Zen AutoLaunch initialization and then selected ZenLocal for DDC.

Recent archived Unreal logs recorded AutoLaunch durations of 37.659 seconds (03:00), 17.103 seconds (03:14), and 3.138 seconds (04:14). Separate UE 5.8.3 worktree runs recorded 27.31 seconds (ModelTier, 2026-10-31) and 29.793 seconds (TideFix, 2026-11-01); those are useful machine-level startup observations, but they are not logs from the active D: editor project. This variability matches the user's report that the warning recurs, while showing that it does not fail every time. In all inspected starts, the installed Zen version matched the engine's bundled version (`5.8.13-202605190912-windows-x64-release-094f62c2`). The Zen server log shows the prior server exiting because its sponsor processes had all exited, which is consistent with Unreal's documented default lifecycle: Zen starts and stops with the editor.

## Paired-launch follow-up

Reviewing the latest project logs changes the likely explanation for the repeated warning. On 2026-11-01 at 07:54:59 UTC, one editor launch started Zen against the shared `Common/Zen/Data` directory. A second editor launch began Zen initialization at 07:55:01 and logged that it was waiting for another process to finish startup/recovery. It timed out at 07:55:33, but immediately afterward its `ZenLocal` DDC reported `Status: OK`; the first launch also reported the HTTP service healthy at 07:55:51. The Zen server log confirms the service was listening on port 8558 from startup. A second pair of project logs around 06:19 shows the same wait/recovery pattern.

This is stronger evidence for overlapping editor startups contending on the shared Zen startup/recovery path than for a damaged or unavailable cache. The warning is still real and the delay is undesirable, but the observed launches recovered and attached to DDC. Avoid starting another editor copy while one is opening; do not clear Zen data based on this warning alone. The process list was unavailable during this inspection, so the log correlation does not identify which launcher started each overlapping editor.

Epic's [UE 5.8 Derived Data Cache documentation](https://dev.epicgames.com/documentation/unreal-engine/using-derived-data-cache-in-unreal-engine?lang=en-US) says Zen opens and closes with Unreal Editor by default and can be configured to remain running. The UE 5.8 release notes specifically recommend `LimitProcessLifetime=false` for workflows where frequent Zen restarts are costly, while warning that automated/build workflows need to account for the longer-lived process.

## Project-level mitigation

`Config/DefaultEngine.ini` now sets `LimitProcessLifetime=False` under
`[Zen.AutoLaunch]`. This is scoped to projects using this configuration and
does not install a Windows service, alter the shared Zen cache, or start a
process immediately. On the next editor launch, Zen should remain available
after the editor closes, avoiding a cold start on a subsequent launch. The
editor was not restarted as part of this change. Verify the next single-editor
launch log shows a short Zen AutoLaunch interval, and confirm Zen remains
healthy after closing the editor. Do not start overlapping editor copies while
the first launch is in progress; the setting reduces cold starts but does not
remove contention if multiple launches concurrently perform recovery.

The first bounded UE Game launch after the change logged the desired
`LimitProcessLifetime=false` context and left `zenserver.exe` running after the
game exited. The local dashboard at `http://localhost:8558/dashboard` returned
HTTP 200, and the process was responsive. This confirms the project setting is
being applied to Game startup and keeps Zen available after Game exit; the next
editor launch is still needed to verify its startup warning and post-editor
lifetime specifically.

**Live-project configuration check (2026-11-01):** the current D: project log
`CaptiveSky_2_2.log` still shows Zen launched with `--owner-pid 20644`, then
becoming healthy after 52.602 seconds. A read-only check of that project's
`Config/DefaultEngine.ini` found no `[Zen.AutoLaunch]` / `LimitProcessLifetime`
entry. The persistent-lifetime mitigation is present in this worktree's
configuration, but has not reached the D: project currently being opened.
Thus the repeated cold start is expected for that live project; the earlier
paired-launch contention can add further delay when starts overlap. No cache
reset or live-project write was made.

**Later live-editor observation (2026-11-01):** a read-only desktop snapshot
showed the Island loaded in an Unreal Editor window, with no Zen warning or
crash dialog visible. This confirms that the editor reached the map on that
observation, but it does not identify the exact project path, measure the
launch duration, or establish that the live D: project's keep-alive setting
has been synchronized.

## Next diagnostic

Once the setting is applied to the D: project, verify a single-editor launch
and confirm the process stays healthy after closing the editor. If that launch
still shows the warning, capture its `Unreal.log` and `zenserver.log` with no
second editor or Unreal automation starting concurrently, then compare the
Zen process start/ready interval and any warnings around it. Do not clear its
cache as a first response.
