# Live Island editor and rendered-play gate — 2026-11-01

## Current observation

The connected UE 5.8 editor is open on the `Island` map and responsive. The
visible viewport is over open water with the island small on the horizon; it is
not a useful composition for judging vegetation or capturing a highlight image.
No PIE or standalone session was started, and the map was not edited.

At the same time, `nvidia-smi` reported 12,282 MiB total GPU memory, 8,319 MiB
used, and 3,679 MiB free. That is below the established 4 GiB reserve for a
rendered playtest, so do not start PIE or another renderer from this state. The
editor's Output Log showed Derived Data Cache maintenance completing, but this
does not establish that the GPU will soon have more headroom or that the Zen
startup warning is resolved.

## Next safe step

Leave the editor and its unsaved state untouched. Re-sample GPU memory when the
device is otherwise idle; only start one bounded rendered session after free
VRAM is at least 4 GiB and the existing system-memory preflight passes. Keep the
30 FPS floor, 30-minute maximum duration, and resident request cap. If the
rendered startup crash recurs, stop and preserve the crash log/dump rather than
retrying the same launch unchanged. Capture a properly framed Island view from
the live editor separately from any PIE/playtest evidence.

This is a live-state checkpoint, not a visual-quality or runtime validation.

## Follow-up sample (14:16 UTC)

The editor remains open and responsive on `/Game/Maps/Island`; the viewport is
now framed at the WoodlandFungusCluster review patch beside the Wind Arch. It
shows the close pale-ochre caps and dense foreground grass documented in the
fungus-view finding, but this editor frame is not a visitor-scale or Game
capture. The Output Log is visible and still contains the read-only Megaplant
dependency report; no PIE or separate renderer was started.

The 14:16 UTC device sample reports 9,154 / 12,282 MiB GPU memory used (2,844
MiB free, 57% utilization, 78 C). Node's OS query reports 10.93 / 31.75 GiB
system RAM free. GPU headroom remains below the 4 GiB rendered-play gate even
though system RAM is available, so keep PIE/Game closed and continue source-side
work. This sample does not establish texture residency, frame rate, or peak
playtime memory.

## Evening gate recheck (19:09 UTC)

The connected editor is still open and reports `All Saved`; a read-only desktop
capture shows the Common Hazel skeletal-mesh preview over the Island editor.
No Zen warning or crash dialog is visible, and no PIE/Game session is running.
The whole-device NVIDIA sample reports 10,754 / 12,282 MiB used (1,244 MiB
free, 68% utilization, 78 C). This is substantially below the 4 GiB minimum
for rendered validation, so keep the 15–30 minute resident play run gated and
continue with source-side work. It is a single device-wide sample, not an Unreal
allocation measurement.

## Read-only recheck — 2026-11-02

Computer Use returned two windows for the installed UE 5.8 editor: the main
`CaptiveSky_2 - Unreal Editor` and a separate `Tree_Common_Hazel_01_A` skeletal
mesh preview. The asset preview reports `All Saved`; the main Island viewport
is visible behind it on the close WoodlandFungusCluster review patch. This is
not a visitor-scale woodland view and does not establish that the worktree's
newer species scatter code is loaded or visually successful. The play control
is idle; no PIE/Game session was started.

`nvidia-smi` reports 10,743 / 12,282 MiB in use (1,255 MiB free), 44%
utilization, and 74 C. This is below the established 4 GiB rendered-session
reserve. Leave the editor and open asset untouched, and keep rendered
validation gated until device-wide headroom recovers; continue with source-side
work in the meantime.

## Fresh viewport and headroom recheck — 2026-11-02 00:32 UTC

After the unchanged Common Hazel preview was minimized, the main Island editor
showed the close WoodlandFungusCluster view unobstructed. The visitor-height
composition is dominated by a broad mushroom cluster and tall, dense grass on
both sides of a narrow ground strip; the Wind Arch and mountain ridge sit behind
it. The frame proves these layers can coexist in the editor, but not that a
visitor can traverse the route, that broadleaf species read distinctly, or that
the current source worktree is loaded in this separate D: project. No PIE/Game
session was started.

The current `nvidia-smi` sample is 10,017 / 12,282 MiB used (2,265 MiB free,
47% utilization, 77 C). Free memory has improved since the previous sample but
remains below the 4 GiB rendered-play gate. Keep the route/profile and 15-minute
resident-feedback run deferred until both GPU and system-memory preflights pass;
do not lower the reserve to force a run. This is one device-wide sample, not an
Unreal allocation or frame-time measurement.
