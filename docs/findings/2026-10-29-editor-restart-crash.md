# Main editor restart fails before project logging

## Result

The user confirmed the Unreal changes were saved and authorized a restart to apply the Nanite Foliage setting. The main editor did not reopen successfully in three launch attempts on 2026-10-29. Each visible result was the Unreal crash window titled “The Game has crashed and will close.” The last attempt included `-log -FullStdOutLogOutput` and produced the same result.

The project configuration still contains `bEnableNaniteFoliage=True` and `r.Nanite.Foliage=True` in `Config/DefaultEngine.ini`, but that is not evidence of a successful post-restart editor load.

## Evidence

- Launch at 04:01:58 returned PID 32736; within 18 seconds no UnrealEditor process remained and the project log had not advanced.
- Launch at 04:03:06 returned PID 30120; within 5 seconds its window title was “The Game has crashed and will close.”
- Launch at 04:04:52 with `-log -FullStdOutLogOutput` returned PID 31948; within 10 seconds the same crash-window title appeared.
- `Saved/Logs/CaptiveSky_2.log` remained timestamped 03:54:01.314 UTC, ending with the prior session's orderly shutdown. No fresh project startup log was created.
- The latest project `Saved/Crashes` directory remained `_0000`, timestamped 03:55:29 UTC. No new project crash context was created by these editor attempts.

## Interpretation and next step

The failure occurs before a fresh project log or project-local crash context is available, so it cannot yet be attributed to Nanite Foliage, the Island map, or VRAM. This evidence is separate from the 03:55 standalone Game attempt's recorded pre-initialization null access violation; the editor attempts did not generate a new crash context proving the same cause.

Do not repeat the same editor launch. Next diagnostic should capture the Windows Application Error/WER event for the 04:01–04:05 UTC attempts (or obtain a fresh CrashReportClient report) and compare its faulting module/exception before changing rendering settings or project content.
