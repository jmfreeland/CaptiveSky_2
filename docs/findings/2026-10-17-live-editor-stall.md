# Main editor stall during Live Coding

## Observation

On 2026-10-17, the desktop app inventory still listed both `Message Log` and
`CaptiveSky_2 - Unreal Editor` windows for the running Unreal Editor process. Two
attempts to bring the project window forward timed out. A shell query for the
process responsiveness flag could not be completed in this session, so there is
no direct `Responding` measurement.

`Saved/Logs/CaptiveSky_2.log` provides stronger evidence of a stalled editor loop:

- The project initialized on 2026-10-16 at `09:46:15`.
- `LogLiveCodingToolset` entered `CompileLiveCoding` at `10:13:37`, immediately
  followed by `LogLiveCoding: Starting Live Coding compile.`
- There is no later Live Coding success, failure, or completion entry in that log.
- Later periodic Derived Data Cache maintenance and UDP messages continue to be
  logged, but every such line retains Unreal's frame counter `[148]`; on the
  2026-10-18 recheck the last recorded activity had advanced to
  `2026-10-18 00:49:21`.

This strongly suggests that the editor stopped advancing during or immediately
after the Live Coding compile request. It does not prove the exact cause or
establish Windows' process responsiveness state; background maintenance logging
can continue while the editor's main loop is stalled.

### Recheck — 2026-10-18 00:57 UTC

The desktop inventory still listed both Unreal windows. A fresh capture for the
project window showed the browser underneath rather than the editor, and two
`activate_window` attempts timed out, including one after refreshing the window
inventory. This leaves the editor inaccessible through Computer Use but does
not independently prove that the process is frozen. The periodic log activity
above is recent, but still carries frame counter `[148]` and is not evidence of
a responsive editor loop.

The newest crash bundle at `Saved/Crashes/_0000/` belongs to a separate
`-NullRHI -ExecutePythonScript` import invocation (PID is reported as zero in the
context) and records a Core null-access startup crash. Do not conflate it with
the GUI editor's state or treat it as a validated restart route.

## Recovery guidance

Leave the process running while unsaved editor work may exist. When the user can
inspect the desktop, first check whether the Live Coding window is still active
and whether the editor accepts input; save if it does. If it remains unresponsive,
preserve the current log and crash/minidump evidence before considering recovery.
Do not terminate the editor without explicit approval because autosave does not
cover every possible unsaved editor change.

## Validation limits

No in-editor screenshot, project save, restart, or fresh Unreal launch was
performed during this check. Repeated project startup attempts have previously
hit a separate Core null-access crash, so restarting is not a validated recovery
path yet.

## Biodiversity follow-up

The local ComfyBlender index already contains 50 candidate packs, including
Tideglass sea thrift, soft rush, marram grass, rock samphire, and sea pea, plus
woodland bluebells, fern, hazel, and birch. The project also has isolated imports
for some review candidates (for example `CoastalCrossLeavedHeath_Trial_20261017`
and `WoodlandFern_Review_20261012`), while `TideglassSeaPea` is not yet imported.
The pipeline README marks these candidates as unplaced/review-only. Therefore,
the next useful biodiversity step is to review and place a suitable existing
candidate in its intended habitat, rather than add another unreviewed export.
That requires a recoverable editor session and bounded in-world visual review.
