# NullRHI resident-feedback RAM guard — 2026-11-01

## Change

The bounded `-NullRHICharacterFeedback` mode can run without consuming
rendered-GPU headroom, but its earlier launcher recipe had no system-memory
preflight or runtime monitor. The mode now requires both
`-RequireSystemMemoryHeadroom` and `-MonitorSystemMemoryHeadroom`. It uses the
existing Windows `GlobalMemoryStatusEx` helper, the existing 4 GiB reserve,
and the existing process-scoped stop after two consecutive low samples. This
does not change the 900-second feedback ceiling, the shared 64-request hard
limit, or resident pacing; the 4 GiB minimum cannot be lowered for this mode,
and the documented example uses a 32-request cap.

The memory check now applies to bounded NullRHI sessions as well as rendered
sessions. GPU headroom remains required only for rendered play. Ordinary
NullRHI diagnostics still require resident thinking to be disabled; the
explicit feedback mode is still the only exception and still requires an
isolated data root, situation logging, Python disabled, and explicit time,
startup, and request caps.

## Validation and limits

- All 58 `Scripts/tests` tests pass, including launcher-boundary assertions
  that feedback mode requires the RAM preflight and monitor.
- `Scripts/tests/Test-SystemMemoryBudget.ps1` passes against the live Windows
  memory API, including the reserve boundary, invalid values, two-sample stop,
  and recovery reset.
- The monitored launcher path itself was not used to launch Unreal in this
  check. No resident prompt was sent and no world-state or resident-memory
  data was written.
- The latest measured GPU headroom was 1,241 MiB, below the 4 GiB gate for
  rendered sessions. The hidden NullRHI mode is not GPU-gated, but an earlier
  engine pre-initialization crash remains unresolved; do not repeat the same
  launch recipe unchanged.

## Next

After a materially different Unreal startup path is verified, run the bounded
900-second NullRHI feedback session with both RAM flags enabled, a separate
data root, situation logging, Python disabled, and a 32-request cap. Review
what each resident chose to share or leave unsaid; treat that sample as
isolated playtest data, not canonical memory. Keep rendered visual/FPS
validation gated separately by GPU headroom.
