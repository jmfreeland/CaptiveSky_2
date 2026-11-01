# Editor restart and GPU headroom recheck — 2026-11-01

After the user confirmed the D: Island editor was saved and approved a restart,
the exact project `D:\Projects - Athena\Unreal\CaptiveSky_2\CaptiveSky_2.uproject`
was closed and reopened. UE 5.8.3 returned to `/Game/Maps/Island`. Startup paused
on the Unreal Zen Storage Server delay prompt; continuing to wait allowed the
editor to load normally.

The fresh D: startup log records `r.Nanite.ProjectEnabled=1` and
`r.Nanite.Foliage=1` at 07:54 UTC. MapCheck completed at 07:56 UTC with zero
errors and zero warnings. The known landscape edit-layer warning recurred:
resolution 4033×4033 exceeds the current 1024×1024 merge batch maximum. No map,
landscape, or Nanite settings were changed during the restart.

At 08:01 UTC, `nvidia-smi` reported 11,675 / 12,282 MiB used, only 323 MiB
free, 47% utilization, and 73 °C. This is a whole-device snapshot; WDDM process
attribution was unavailable, so it does not assign the memory to Unreal. The
editor viewport remained an offshore-facing edit view, not a runtime vegetation
or player view. No PIE/Game session, character feedback, or rendered profile was
started because headroom is far below the 4 GiB reserve.

## Decision and next investigation

Keep the existing 30-FPS floor and do not increase or cut vegetation populations
from this sample. The prior editor-only `ListTextures` inventory recorded about
1.6 GiB in transient render targets, including 4K `WaterBrushManager` targets;
the installed UE Water header confirms these are transient properties but the
engine installation has no implementation source to establish their allocation
lifetime or runtime role. That inventory cannot be treated as the cause of the
PIE memory-pressure warning. Next, obtain a fresh allocation inventory only
when the editor can report it without entering PIE, correlate named render
targets to their owning systems, and test a reversible resolution change only
after the owner and visual effect are verified. Resume a bounded 15–30 minute
resident-feedback session only after both system RAM and GPU headroom are safe,
with live 30-FPS and memory monitoring.
