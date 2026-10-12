# North woodland Rat-hollow candidate review — 2026-10-12

## Scope and result

The candidate at `(-104500, 105000, 2337.5)` cm was tested only in the
disposable `/Game/Maps/Island_RatHollowCandidateTrial` clone. The corrected
RatHollow art-pass mesh is at scale `0.72`; the current clone faces west
(actor yaw `0°`) so its local `-X` entrance points toward the west-side
approach. The earlier `180°` orientation faced east. The saved Island and
`Agents/Agent_Rat_01/places.json` were not changed.

The site is **not approved as Fenrus's home**. The existing capture labeled
“Rat Height” is actually about 102.5 cm above the hollow's ground-centre origin,
well above the Rat's roughly 31 cm scaled body height. In that elevated view,
broad foreground plants cross the entrance and make the dark opening hard to
read. The human-height approach shows a broad, repetitive ground-cover slope
rather than a distinct, hidden microhabitat. This is rejection evidence for the
current composition, not evidence that the home concept or all north-woodland
sites should be abandoned.

## Bounded runtime evidence

The real-RHI Game preview reused the already-warmed local DDC, reached
world-ready after 83.4 seconds, ran with resident thinking disabled and zero
model requests, then exited normally at its 90-second cap. It captured two
viewpoints and a later repeat of the rat-height view. The 09:00 rat-height
capture was still preparing shaders and streaming and is not a settled visual
or texture-budget sample.

At 09:28, the settled elevated “Rat Height” capture reported a 4096 MiB pool,
958.56 MiB Required Pool, 922.09 MiB Visible Mips, 29.72 MiB Hidden Mips, and
zero in-flight requests (about 23.4% of capacity). In a second run with the
entrance facing west, the 09:14 human-approach capture reported 959.39 MiB
Required Pool against the same 4096 MiB capacity. Neither sampled view shows a
streaming-pool overage. They cannot rule out the user's recurring warning in
the editor, on other routes, or under a different texture profile.

The west-facing pair captured the log from both sides, but its rat-height
camera still used the old elevated coordinates. The corrected camera is 30 cm
above ground and about 2 m from the entrance (`ViewpointsFaceWestRatPOV.json`).
An elevated, bounded 60-second run reached world-ready in 27.9 seconds, ran
with resident thinking disabled and zero model requests, then exited normally.
At true Rat eye height, the opening is still mostly swallowed by broad leaves;
from the human approach, the hollow itself disappears as intended. This is a
promising concealment relationship, but not yet a readable Rat-scale
microhabitat.

The Rat-POV frame was at 09:00 while texture requests were still in flight
(128.05 MiB), so its `Required Pool` counter is not used as settled evidence.
The 09:14 human approach reported 959.39 MiB Required Pool against the 4096
MiB pool (about 23.4% capacity), with no streaming-pool overage. This again
does not explain the user's warnings in the editor or other views.

This launch recorded hundreds of D3D12 PSO-creation waits while the first
viewpoints warmed. It is a startup/shader-cache cost, not a steady-state FPS
benchmark. Reusing the warm DDC avoided the prior five-minute cold-cache
timeout.

## Evidence and next step

- Run: [`Codex_RatHollowCandidateTrialWarmRHI_20261012.log`](../../Saved/Logs/Codex_RatHollowCandidateTrialWarmRHI_20261012.log)
- Rat height, settling: [`001`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/Screenshots/001_North_Woodland__Rat_Height.png)
- Human approach: [`002`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/Screenshots/002_North_Woodland__Human_Approach.png)
- Rat height, settled: [`003`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/Screenshots/003_North_Woodland__Rat_Height.png)
- West-facing pair: [`rat-side`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsFaceWest/001_North_Woodland__Rat_Height.png), [`human-side`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsFaceWest/002_North_Woodland__Human_Approach.png), [run log](../../Saved/Logs/Codex_RatHollowCandidateFaceWestRHI_20261012.log)
- True Rat-eye view: [`rat POV`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsRatPOVElevated/001_North_Woodland__Rat_POV.png), [`human approach`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsRatPOVElevated/002_North_Woodland__Human_Approach.png), [viewpoints](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ViewpointsFaceWestRatPOV.json), [run log](../../Saved/Logs/Codex_RatHollowCandidateRatPOVElevatedRHI_20261012.log)

The next useful art pass should test a small, deliberate approach opening and
natural shelter framing around the entrance, then compare true Rat-eye and
human-height views without the profiling overlay. If clearing the opening makes
the home conspicuous from human height, find another candidate. Rat capsule
traversal and nav clearance remain unverified; keep the canonical place list
empty until those checks pass.

## Source-only approach-detail trial (2026-10-12)

Two reversible Blender previews tested whether the entrance problem could be
solved inside the prop. A woody root sweep made the hollow read like a built
doorway; a low disturbed-soil/litter patch read as a detached dark slab in the
isolated asset preview and expanded the prop's footprint without showing how it
would blend into the Island soil. Neither treatment is accepted. The original
`rathollow.py` and clearance tests were restored unchanged; the pre-existing
`RatHollow_TornEntranceV2` local export was regenerated from that baseline. The
new renders stay under the ignored `Saved/CompileScratch/RatHollowMouthFrameTrial_20261012/`
folder and were not imported or placed in either map.

This narrows the next experiment: adjust the disposable site's local understory
composition/clearance, not the hollow mesh. Keep a few small leaf-litter and
moss details close to the entrance, then compare true Rat-eye and human-height
views in the actual woodland material context. Do not author the home's
canonical place record until that pair, Rat capsule clearance, and NavMesh are
verified.

## Tag-driven understory clearance prototype (2026-10-12)

`AIslandWeather` recognizes the opt-in `RatHollowClearance` tag. It keeps a
72 cm mouth radius and 150 cm approach lane, 100 cm wide, open in local `-X`,
then expands the exclusion only by each selected plant mesh's scaled horizontal
footprint so broad leaves do not overhang the Rat's entrance. Smaller cover
remains eligible nearby. The footprint table is built once and queried only in
worlds with a tagged hollow, so the ordinary Island scatter path avoids the
per-instance lookup. The saved Island and Fenrus's canonical place record remain
unchanged. The trial-only tag script checks the loaded map, actor label, and
exact mesh object path before saving the disposable clone; a read-only inventory
script records that actor's mesh and tags.

UE 5.8.3 `CaptiveSky_2Editor` built successfully in the isolated
`Saved/CompileScratch/Claude_Props` target. The focused
`CaptiveSky2.Agent.GroundCover` automation passed after the footprint-aware
change (`Saved/Logs/Codex_RatHollowGroundCoverFinal_20261012.log`). The tag was
applied only to `Content/Maps/Island_RatHollowCandidateTrial.umap`; its
pre-change byte-identical backup is
`Saved/Backups/RatHollowClearanceTrial_20261012/Island_RatHollowCandidateTrial_PreClearance.umap`.

A 60-second real-D3D12 Game capture reached the world-ready marker, then exited
normally with resident thinking disabled and zero model requests. The scatter
logged 11 candidate plants skipped. The true Rat-eye view now shows the hollow
opening clearly, though a broad leaf still crosses its upper edge. The
human-height view retains nearby plant groupings but reads as a conspicuous bare
patch on a repetitive slope. This is a useful entrance-clearance prototype,
not an approved hidden home site; capsule traversal, NavMesh, and a better
human-scale setting remain open. Captures:
[`Rat-eye`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsClearanceFootprint/001_North_Woodland__Rat_POV.png),
[`human approach`](../../Saved/Playtests/Codex_RatHollowCandidateTrial_20261012/ScreenshotsClearanceFootprint/002_North_Woodland__Human_Approach.png),
[`runtime log`](../../Saved/Logs/Codex_RatHollowClearanceFootprint_20261012.log).

The test run used the project's intended 4096 MiB texture pool via an explicit
startup CVar, but did not enable `stat streaming`; it does not reproduce or
disprove the recurring over-budget warning. Unreal MCP remains unavailable.
