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
