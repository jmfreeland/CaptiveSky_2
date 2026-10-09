# Avoid redundant hidden footprint transform writes (2026-10-10)

The persistent trail subsystem keeps its existing 220-slot wet-footprint ring
and refreshes it at the existing 10 Hz cadence. Previously every refresh wrote
all 220 instance transforms, including unused or already-hidden slots. Each
slot now remembers whether its footprint is rendered: active visible prints
continue receiving transforms as they shrink, newly visible prints are
written, and a print that dries or expires receives one transform to hide it.
Hidden slots are then skipped until they are reused.

This preserves the print mesh, positions, wetness response, lifetime, ring size,
and draw behavior. It reduces instanced-mesh transform writes when few or no
prints are visible; it does not change the number of marks or vegetation and is
not claimed as an Island-wide frame-rate fix. The subsystem still scans the
bounded ring on its existing refresh interval.

UE 5.8.3 built the editor target successfully. The focused
`CaptiveSky2.Agent.IslandTrail` automation passed, including assertions for
idle hidden slots, animated visible prints, initial reveal, and one-time hiding
after drying or expiry. The headless run used NullRHI, disabled resident
thinking and Python, allowed zero model requests, and used an isolated world
data root. The log reports `Result={Success}` and exit code 0:
[`Codex_TrailPrintRefresh_Automation_20261010.log`](../../Saved/Logs/Codex_TrailPrintRefresh_Automation_20261010.log).

A matched real-RHI frame-time comparison has not been run; the expected benefit
is limited to reducing transform-update work during refreshes with hidden
slots. No Content, map, or persistent world-state data was changed.
