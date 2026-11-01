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
