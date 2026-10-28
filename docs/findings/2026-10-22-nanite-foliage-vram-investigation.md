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

## Restart verification (2026-10-28)

After the user confirmed the editor was saved and approved a restart, the
standalone preview was stopped and the `CaptiveSky_2` editor was closed and
reopened on the Island map. The reopened editor showed `All Saved`. Querying
`r.Nanite.Foliage` in the live console returned `1` with `LastSetBy:
ProjectSetting`, confirming the restart applied the project setting. This
confirms CVar activation only: it does not yet establish that the Nanite
assemblies render correctly, improve frame time, or reduce peak VRAM. No paired
memory/frame-time capture or player traversal was performed. The runtime view
showed a populated herbaceous layer and several tree silhouettes at night. The
editor log reports the preview ran for 31.0 real seconds and made 4 model
requests, with none still in flight when it ended. That single view does not
identify species count or prove a Nanite visual/performance change, and no
memory/frame-time reading was collected. The landscape edit-layer resolution
warning remains visible in the editor log and is unrelated to Nanite activation.

**Second bounded runtime check (2026-10-28):** after the restart, the editor
opened the saved Island and finished landscape texture streaming. The live log
again records `r.Nanite.Foliage=1` (`LastSetBy: ProjectSetting`). Before
Standalone play, `CaptiveSky.Agent.DisableThinking=1` was set for a zero-request
visual check. The play-session watchdog reported a 1,800-second / 120-request
ceiling; the session was stopped after 95.6 seconds and logged 0 model requests
and 0 in flight. Its night-time view showed a densely vegetated foreground and
several distinct tree silhouettes, but it is not a matched daytime composition
or a species census. No FPS, peak VRAM, or water traversal measurement was
collected, so neither the 30-FPS target nor the earlier memory-exhaustion
warning is resolved by this run.

Two asset/landscape warnings are now directly recorded during startup/runtime:
the landscape edit-layer merge needs a 4033×4033 batch while the current maximum
is 1024×1024, and `/Game/Materials/M_Island_Textured_Wet` requested texture index
21 of 20. The latter belongs to the in-flight landscape material work; do not
edit its authored asset here. Diagnose the parameter/texture indexing with its
owner before changing material quality or landscape batch settings. The editor
remains open on the Island map with no map save or material/config changes from
this check.
