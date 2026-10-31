# Idle Island editor: RHI and composition snapshot (2026-10-31)

## Observation

The main `D:/Projects - Athena/Unreal/CaptiveSky_2` editor was open on the
saved Island at approximately 17:32, not in PIE. Read-only `stat streaming`
reported a 4,096 MiB texture pool with 3,726.57 MiB in use (91%), 354.34 MiB
of non-streaming mips, and no in-flight texture requests. The prior texture
streaming audit's settled Game views were around 1,000 MiB Required Pool; this
idle editor sample is not directly comparable to those Game captures and did
not display the user's video-memory exhaustion warning.

`stat D3D12RHI` reported approximately 2,953 MiB Render Target 2D memory,
1,623 MiB UAV texture memory, and 671 MiB Texture 2D memory. It also listed
6,076 MiB of *uncommitted* reserved buffer address space; that is not committed
GPU memory. The RHI categories are separate counters and must not be added as
though they formed a total or assigned to individual assets from this view.
The red `PROFILING WITH AI LOGGING ON!` overlay is part of this stat display,
not evidence of a memory warning.

At the same time, `nvidia-smi` reported 10,050 MiB of 12,282 MiB in use and
55% GPU utilization. WDDM exposed no process rows, so this global reading
cannot be attributed to Unreal alone. Output Log still showed one queued and
one asynchronous package, repeated 100 ms waits for PSO creation, and a
landscape edit-layer merge warning: the 4033×4033 landscape exceeds the
current 1024×1024 merge setting. These are observed startup/rendering signals,
not proof of the user's PIE memory failure's cause.

The editor camera shows an open hillside with little low cover and a few
nearby yellow-leaved trees that read similarly. This supports the user's
composition concern at this angle, but it is not an Island-wide species count.
The editor remained **All Saved**; no map, asset, or project setting was
changed. Diagnostic stat overlays were cleared afterward.

## Next diagnostic

After the guarded play-session cap and feedback changes are integrated in the
main project, reproduce the exact gameplay camera that shows the exhaustion
warning. Capture `stat streaming`, `stat D3D12RHI`, and the relevant texture
contributors before changing pool size, foliage count, scalability, or
landscape settings. Keep this editor-idle snapshot separate from Game/PIE
measurements. Do not treat a 91% editor streaming-pool occupancy as proof of
either a texture-pool overage or the whole-GPU warning.

## Post-restart Nanite Foliage check (2026-10-31)

After the user-approved editor restart, the correct `D:/Projects -
Athena/Unreal/CaptiveSky_2` project reopened on the saved Island. Querying
`r.Nanite.Foliage` in its Output Log returned `1` with `LastSetBy: ProjectSetting`;
the editor status remained **All Saved**. The feature toggle is therefore active
in this editor session. This does not prove that any particular imported
Megaplant asset is eligible for or actually using Nanite Foliage, nor that the
feature improves this project's frame time or memory use. The idle viewport
still shows an exposed hillside and a few nearby similar-looking tree forms;
it is one editor angle, not a runtime census of the four broadleaf species.

Next, inspect the four tree meshes' Nanite eligibility and runtime rendering,
then compare the same fixed Game view and a separate player traversal with
streaming/RHI measurements. Keep the user's over-budget warning distinct from
this editor-idle setting check.
