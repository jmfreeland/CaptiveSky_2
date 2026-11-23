# Island texture streaming budget baseline

## Result

The bounded 2026-10-11 Island viewpoint sweep did not show a texture-pool
shortage in its sampled runtime views. The project setting was
`r.Streaming.PoolSize=4096` MiB with `r.Streaming.LimitPoolSizeToVRAM=1`. The
`stat streaming` screenshots reported a required pool of approximately 837 MiB
at Shore Approach, 979 MiB at Inn From Path, 995 MiB at Inn Guest Book, 993 MiB
at Tideglass Ground Detail, and 978 MiB at Tideglass Dragonfly Close. These are
well below the configured 4096 MiB capacity.

In `stat streaming`, the `Streaming Pool` row reports the configured pool
capacity and therefore reads 4096 MiB / 100% in these captures; that row by
itself is not evidence that 4 GiB of texture demand is resident or that the
budget is exceeded. Compare the `Required Pool` / `Wanted Mips` figures with
the pool capacity and look for an explicit `TEXTURE STREAMING POOL OVER` warning.

Evidence is in
`Saved/Playtests/Codex_TextureBudgetViewSweep_20261011/Screenshots/` and
`Saved/Logs/Codex_TextureBudgetViewSweep_20261011.log`. The sweep ran with the
normal project pool, loaded the Island, disabled resident thinking, capped
runtime to 180 seconds and model requests to zero, and collected seven authored
views. Its screenshot overlays were captured during profiling, so use them as
budget diagnostics, not clean presentation images.

## Limits and next check

This does not contradict the user's recurring report of frequent over-budget
messages, reiterated on 2026-10-12. The sweep did not include an editor-heavy
session, a long free-roam route, or all high-resolution Fab assets/views. Older
probe runs explicitly set the pool to 1000 MiB and should not be treated as
normal runtime evidence. On 2026-10-12, no Unreal Editor process was running
and the Unreal MCP transport at `127.0.0.1:8000` was unavailable, so the live
editor viewport could not be sampled. No literal `TEXTURE STREAMING POOL OVER`
or `Over Budget` line was found in the saved logs; a screenshot of the actual
warning and `stat streaming` in that session remain necessary to distinguish a
post-fix texture-pool overage from editor/whole-GPU memory pressure.

The same sweep log contains one notable texture-build signal:
`/Game/PN_FoliageCollection/Textures/grassTextures/MasterTextures/springGrass_Opacity_8k`
was built as an 8192x8192 `TFO_AutoDXT` texture with a logged “Required Memory
Estimate” of 1537 MiB. This is **not** the runtime `Required Pool` measurement
and does not prove the texture consumes that much GPU memory while playing; the
viewpoint captures still measured about 1 GiB wanted. The detailed audit in
[`2026-10-11-texture-streaming-pool.md`](2026-10-11-texture-streaming-pool.md)
shows this texture was already changed in a reversible local trial to
`MaxTextureSize=4096` with group-generated mips. Its original package is backed
up, and the saved Island was not changed. Treat the 1537 MiB log line as an
unexplained build-time estimate, not as a live 8K residency reading or an
untreated asset setting.

Large on-disk source packages are not resident VRAM usage. The detailed audit
also identifies non-streaming spruce impostor textures and distinguishes
Island-reachable candidates from merely imported assets; use it as the current
asset-level record rather than inferring residency from package size.

When the warning recurs, capture its exact text and `stat streaming` in the same
editor/game session. Compare `Required Pool` to the effective pool setting,
then use `ListTextures` (and the editor texture resource list) to identify the
largest resident textures before changing global pool size or reducing asset
quality. Avoid raising the pool based only on the `Streaming Pool 100%` row;
that can consume VRAM without fixing an oversized or unexpectedly pinned asset.
The useful next evidence is still the exact user warning view, captured after
streaming settles, followed by `ListTextures` in that same session. Compare its
largest wanted/non-streaming contributors against the detailed audit before
changing imported Megaplant or foliage settings.

## Megaplant library reference audit (2026-10-12)

The read-only UE 5.8.3 Asset Registry audit enumerated 19 Texture2D assets under
`/Game/Megaplant_Library` across Common Hazel, European Aspen, European Beech,
and Norway Maple. None had a dependency path to `/Game/Maps/Island`; all had
group-based mips and `NeverStream=false`. The 16 bark/foliage color and normal
maps enable virtual-texture streaming; the three PVE displacement maps do not.
The four imported species therefore add about 630 MiB of package data on disk,
but their textures are not currently referenced by the saved Island map and are
not an identified cause of its runtime streaming-pool warnings. This says
nothing about editor asset previews or future placement of those trees.

Audit log:
`Saved/Logs/Codex_MegaplantTextureAudit_Final_20261012.log`. It used
`UnrealEditor-Cmd` with `-NullRHI`, `-DDC-ForceMemoryCache`, and
`-ddc=NoZenLocalFallback`; the map and assets were not saved. The ordinary
installed DDC path failed before the audit could start, so do not reuse that
launch form. The expanded diagnostic script now discovers Texture2D assets
under the Megaplant library automatically.

## Island landscape 8K texture trial and cap (2026-10-12)

A targeted UE 5.8.3 Asset Registry walk found that the Fab Megascans 8K ORM
texture and 8K displacement texture for
`Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs` are referenced by the saved
Island through
`M_Island_Textured_ShoreSubstrate -> MI_Island_Landscape -> /Game/Maps/Island`.
Both assets were configured with `MaxTextureSize=0`, group-generated mips,
streaming enabled, and virtual-texture streaming disabled. This distinguishes
them from the separate imported Megaplant library, which has no Island path.

A disposable 4K trial duplicated both textures and the landscape parent under
`/Game/Generated/TextureBudgetTrial_20261012`; only the duplicates received
`MaxTextureSize=4096`. The two material samples were rewired in the duplicate,
which compiled without diagnostics. A same-camera/hour 1600x900 close-up A/B
(`TideglassGroundDetail`, 11:00) was visually indistinguishable at this view.
The texture-build log estimate fell from 4481 to 1537 MiB for the ORM and
6059 to 2305 MiB for the displacement copy (about a 66% reduction in those
build-time estimates). These estimates are **not** GPU-resident or runtime
streaming-pool measurements.

Based on that result, both original texture packages now have
`MaxTextureSize=4096`; compression, mip-generation, and streaming settings were
preserved. The original 8K source maps remain intact. Byte-for-byte package
backups (SHA-256 verified before editing) are in
`Saved/Backups/LandscapeTextureBudget4K_20261012/`. The authored landscape
material compiled with no diagnostics, and a real-RHI offscreen capture using
the authored parent rendered successfully. Neither `MI_Island_Landscape` nor
`/Game/Maps/Island` was saved or edited. The reversible trial assets remain in
`/Game/Generated/TextureBudgetTrial_20261012`.

The next bounded game preview with post-change `stat streaming` did not reach
Island: Unreal remained at `Build.bat -Mode=ValidatePlatforms` until the
launcher terminated only that instance at its 600-second startup cap. It
produced no post-change runtime-pool measurement. Earlier real-RHI views at the
same 4096 MiB pool reported roughly 837–1009 MiB Required Pool and zero
over-budget at the sampled cameras; the user's recurring warning remains
unreproduced. Do not claim this texture cap resolves it everywhere. When the
warning recurs, capture its exact view and streaming stats; compare `Required
Pool` and `Over Budget` before considering further texture or pool changes.

The single-view repro route is `Config/CodexTextureBudgetTideglass.json`. The
read-only texture audit, clone trial, and gated apply scripts are
`Scripts/Inspect-FabMegascansTextureReferences.py`,
`Scripts/Test-LandscapeTextureBudget4K.py`, and
`Scripts/Apply-LandscapeTextureBudget4K.py`.

**Scratch-project caveat (2026-10-12):** the isolated `Claude_Props` scratch
project's copied `Config/DefaultEngine.ini` predates the main project's 4 GiB
`[SystemSettings]` pool setting. A bounded candidate-site Game therefore
started at Unreal's 1000 MiB default; an explicit startup CVar changed it to
4096 MiB before the first requested screenshot. The run's log confirms both
values, but it did not enable `stat streaming` and is not an over-budget
measurement. When using scratch projects for future texture diagnosis, copy the
current project settings or explicitly set and verify the intended pool before
comparing results. Evidence:
[`Codex_RatHollowClearanceFootprint_20261012.log`](../../Saved/Logs/Codex_RatHollowClearanceFootprint_20261012.log).

**Post-cap actual-Island streaming stats (2026-10-12):** a 60-second,
zero-model-request real-RHI Game run loaded `/Game/Maps/Island` from the isolated
scratch build, forced the intended 4096 MiB pool, and captured the established
Tideglass close-up twice as streaming settled. Both `stat streaming` overlays
reported `Required Pool = 993.00 MB` against `Pool Capacity = 4096.00 MB`; no
over-pool warning appeared. The first frame still showed six shaders preparing,
while the later frame was settled, so treat the latter as the stronger reading.
This is post-4K evidence for this Game camera, not proof about the user's editor,
all world views, or whole-GPU memory. Screenshots:
[`11:00, settling`](../../Saved/Playtests/Codex_TextureBudgetPostCapActualIsland_20261012/Screenshots/001_Texture__Budget__Closeup.png),
[`11:14, settled`](../../Saved/Playtests/Codex_TextureBudgetPostCapActualIsland_20261012/Screenshots/002_Texture__Budget__Closeup.png);
run log:
[`Codex_TextureBudgetPostCapActualIsland_20261012.log`](../../Saved/Logs/Codex_TextureBudgetPostCapActualIsland_20261012.log).

**Post-cap runtime recheck attempt (2026-10-12):** a one-view Tideglass Game run
was configured for 45 seconds, zero model requests, resident thinking disabled,
and a 120-second startup guard. It did not reach the Island. The startup log
records `Build.bat -Mode=ValidatePlatforms` followed by
`UBT AutoSDK ReturnCode: -532462766`; the launcher then terminated only its
Game process tree when the world-ready marker did not arrive. No `stat
streaming` runtime counters or screenshot were produced, and a post-run process
check showed no remaining Unreal process. Log:
[`Codex_TextureBudgetPostCap_20261012.log`](../../Saved/Logs/Codex_TextureBudgetPostCap_20261012.log).
This repeats the known Turnkey/AutoSDK startup failure and provides no evidence
for or against the 4K texture cap's effect on runtime pool pressure.

**Expanded post-cap coverage attempt (2026-10-12):** a 300-second,
zero-model-request real-RHI run was prepared to visit all 13 viewpoints in the
current `Config/IslandViewpoints.json` with `stat streaming` enabled and a
180-second startup guard. It never reached world-ready: its log stops just
after loading `TurnkeySupport` and launching `Build.bat -Mode=ValidatePlatforms`.
The bounded launcher terminated only the Game process tree. No screenshot,
camera-level texture-pool sample, or Island save resulted. A read-only query of
the Windows Application log for the prior three hours returned no matching
`.NET Runtime` / `Application Error` event, and no Unreal or dotnet process
remained afterward. This adds evidence of an intermittent UE/Turnkey startup
stall, not evidence for or against texture over-budget after the 4K cap. Log:
[`Codex_TextureBudgetPostCapCoverage_20261012.log`](../../Saved/Logs/Codex_TextureBudgetPostCapCoverage_20261012.log).

**Later close-up warning and current editor setting (2026-10-16–18):** the
45-second Periwinkle close-up on 2026-10-16 displayed
`TEXTURE STREAMING POOL OVER 4.449 MiB BUDGET`. Its runtime log resolves the
warning context: the session applied `[TextureQuality@3]` with
`r.Streaming.PoolSize=1000` and `r.Streaming.LimitPoolSizeToVRAM=0`, then logged
`Texture pool size now 1000 MB`. This is a genuine over-budget capture, but it
is not evidence that the current 4096 MiB setup was exceeded. The capture ran
from the `Claude_Props` scratch project; its current `DefaultScalability.ini`
was refreshed on 2026-10-18 and now contains the 4096 MiB / VRAM-clamp settings
at all four texture-quality tiers. Do not infer why the older run had the
1000 MiB setting without its then-current config snapshot.

The open main editor's 2026-10-18 startup log confirms
`r.Streaming.PoolSize=4096`, `r.Streaming.LimitPoolSizeToVRAM=1`, and
`Texture pool size now 4096 MB`. It has no `stat streaming` Required Pool or
Over Budget counters, so the user's current-session warning remains
unreproduced at the corrected setting. Next useful evidence is a same-session
capture of the exact warning view plus `stat streaming` after streaming settles;
compare Required Pool and Over Budget against the effective 4096 MiB pool
before changing source texture settings again. Evidence:
[`PeriwinkleCloseup.log`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Closeup/Logs/PeriwinkleCloseup.log),
[`warning screenshot`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Closeup/Screenshots/006_Perwinkle__Close.png),
and [`current editor log`](../../Saved/Logs/CaptiveSky_2.log).
