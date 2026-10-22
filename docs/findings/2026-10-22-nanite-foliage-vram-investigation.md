# Nanite foliage and VRAM state check (2026-10-22)

## Current editor state

- The open editor still displays Unreal's Nanite Assembly notice saying that
  Assemblies are disabled and may not appear as expected.
- Project Settings API reports `bEnableNaniteFoliage=true`, while the live
  `r.Nanite.Foliage` CVar is `0`. This is consistent with the saved setting not
  having taken effect in this editor session; it does not establish that the
  imported tree assemblies themselves are valid.
- The project texture streaming pool remains `4096` MiB with
  `r.Streaming.LimitPoolSizeToVRAM=1`. The current editor log contains startup
  pool sizing only, with no new pool-over or RHI exhaustion entry. An earlier
  same-day Island PIE capture did show Unreal's “Video memory has been
  exhausted (179.7 MB over budget)” overlay, so the warning remains real even
  though it was not reproduced in this check.
- Host-wide `nvidia-smi` reported 9,465 / 12,282 MiB used (2,533 MiB free).
  Per-process GPU records returned “Insufficient Permissions”; this figure
  cannot be attributed to Unreal or to a particular texture/mesh. Do not use it
  alone to tune the Island's texture pool.

Epic's UE 5.8 documentation says enabling Nanite Foliage requires an editor
restart, and describes Nanite Assemblies as a way to reduce repeated foliage
geometry and streaming memory. The feature is experimental, so the next
validation gate is to restart only after the user confirms the editor has no
unsaved work, then confirm `r.Nanite.Foliage=1`, check whether the notice clears,
and measure the same Island view before and after. See [Nanite Foliage]
(https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage)
and [Nanite Assemblies]
(https://dev.epicgames.com/documentation/unreal-engine/nanite-assemblies).

## Guardrails

- No config, pool size, quality, or imported asset settings were changed during
  this read-only check.
- Do not increase the global texture pool until `stat streaming` captures the
  required pool and an explicit over-budget condition in the warning view.
- Keep world playtests short until the assembly setting and GPU headroom are
  measured; the host-wide snapshot is not a substitute for a paired editor/PIE
  allocation sample.

## Runtime tree follow-up

A further short Simulate capture showed a much denser herbaceous layer than the
pre-play editor viewport, with many tree silhouettes across the slope. While
PIE was active, the maple, aspen, and beech instanced-skinned components each
referenced their distinct Megaplant asset and a distinct `DynamicWindData`
transform provider; all three were visible and used 8,000–12,500 cm cull
distances. After PIE stopped, the editor-world components reported no skinned
asset and zero instances, so the editor's non-PIE viewport is not a census of
the generated gameplay forest.

`CaptureAssetImage` thumbnails for all three Megaplant meshes showed bare
branch silhouettes. The active editor still had `r.Nanite.Foliage=0` and the
Assemblies-disabled notice. This is consistent with the warning that the assets
may not appear as expected, but the thumbnail alone does not prove whether
Nanite activation will restore their foliage. The editor was left open pending
user confirmation that it is safe to restart; no player-input traversal test or
memory comparison was performed in this Simulate pass.
