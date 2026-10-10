# D3D12 editor device hang — 2026-10-10

## Evidence

The current `Saved/Logs/CaptiveSky_2.log` records an Unreal GPU crash at **2026-10-10 13:31:37 local time** (12:31:37 UTC). The editor process was PID 26356, running UE 5.8.3 Development on Direct3D 12. The device was removed with `DXGI_ERROR_DEVICE_HUNG`; NVIDIA Aftermath classified it as `Error_DMA_PageFault` / `PageFault` on the graphics engine. The adapter itself was not reset, while the engine was reset.

The recorded fault address is `0x001813d70117731d`. Aftermath could not map it to a resource or shader source: it reports no resource info or marker info, and DRED reports no breadcrumb head or page-fault data. The RHI's own tracker found no active or recently released resource in the nearby 16 MB range. The shader is identified only as a pixel/fragment shader (`MainPS` / `fragment_02`); there is no useful source mapping.

At the last reported frame, local video-memory use was 8,323.87 MB against a 9,188 MB local budget (about 90.6%). This is high but below the reported budget, so the log does **not** establish a VRAM-budget exhaustion cause. The crash context says the editor had been running for about 45 minutes and its activity hint was `StaticMeshEditor_Properties`; it does not identify PIE or a resident simulation as the trigger. The Aftermath dump is local and gitignored at `Saved/Logs/D3D12.0.2026.10.10-13.31.37.nv-gpudmp`.

## What this does and does not tell us

This is distinct from the recurring `dotnet.exe` dialog with exception `0xe0434352`. That exception code does not appear in this editor log. This evidence confirms a D3D12 device hang/page fault, but it does not identify a faulty asset, material, shader, driver, or project subsystem. In particular, it is not evidence that the recent foliage work caused the crash.

## Follow-up

Do not make a broad rendering-setting or asset change based on this single unattributed dump. If the GPU hang recurs, preserve the matching log and dump, note whether the editor was in a static-mesh editor, viewport, PIE, or build, and capture the exact asset/view being used. Then compare a bounded reproduction in an isolated project, changing one rendering subsystem at a time. Investigate the .NET exception separately using its own timestamp, owning process, and Windows application event; do not treat this GPU dump as its explanation.
