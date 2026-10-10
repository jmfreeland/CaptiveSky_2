# Listening Stones surface override (2026-10-09)

The existing Game close-up at
[`001_Stone_Presentation_Close.png`](../../Playtests/Codex_StonePresentation_20261007/GameScreenshotsTintFinal/001_Stone_Presentation_Close.png)
still shows the transient cairns with strongly mottled black-and-white faces.
The `SM_Rock` asset preview confirms that this contrast belongs to its default
surface. The presentation created a warm-gray dynamic instance but assigned it
only to material slot 0, without checking for other mesh slots.

`AListeningStonePresentation::BuildStoneForms` now assigns that instance to
every static material slot on `SM_Rock`. If the known engine surface or dynamic
instance cannot be created, the transient replacement fails and the unchanged
map proxies remain visible. The focused regression logs the asset's slot count
and checks every slot on the transient component for a dynamic override.

## Verification and limits

- The two touched translation units compiled successfully with the cached UE
- A full UE 5.8.3 scratch editor build succeeded (47 actions), including
  `IslandListeningStonePresentation.cpp` and its test. `UBT AutoSDK ReturnCode`
  was 0.
- `CaptiveSky2.Agent.ListeningStonePresentation` passed. The regression logs
  one `SM_Rock` material slot and verifies it resolves to the transient dynamic
  stone surface. Log:
  [`Codex_ListeningStones_Override_Automation_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_ListeningStones_Override_Automation_20261009.log).
- The first editor-only viewpoint capture showed the unchanged blue map proxies,
  as expected: that preview does not start Game and therefore does not exercise
  the transient Game presentation. A bounded real-RHI Game run then loaded the
  current scratch binaries, replaced the proxies with the runtime cairns, and
  saved a warmed noon frame at
  [`002_Stone_Presentation_Close.png`](../../Saved/CompileScratch/Claude_Props/Saved/GameShots/Codex_ListeningStones_Override_Noon_20261009/002_Stone_Presentation_Close.png).
  It used an isolated data root, disabled thinking, a zero-request ceiling, and
  a 60-second real-time cap; it exited normally after 60.2 seconds with zero
  model requests. Log:
  [`Codex_ListeningStones_Override_GameNoon_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_ListeningStones_Override_GameNoon_20261009.log).
- The all-slot override is in effect, but the warmed frame still reads as
  strongly light/dark stacked rock. Since `SM_Rock` has only one material slot,
  extra slot coverage was not the visual bottleneck. The current frame does not
  distinguish whether the remaining contrast comes from the base surface,
  direct lighting, or self-shadowing at layer seams. Next: evaluate the
  candidate surface and the stack silhouette separately under matched daylight
  and shadow views before changing the tall landmarks again. Keep the imported
  small mossy `StoneCairn` prop separate; it is too wide to scale into these
  narrow 2.2–3.0 m proxies.

No map or Content asset was changed; the presentation remains transient and the
saved proxy actors remain authoritative. Scratch output uses
`Saved/CompileScratch/Claude_Props`; no user world state or open editor assets
were modified.

## Procedural standing-stone iteration (2026-10-09)

The stacked Starter Content rocks still read as a cartoon cairn when stretched
across these narrow 2.2–3.0 m proxies. The transient presentation now builds one
seeded, tapered procedural stone per proxy, with a small uneven cap instead of
a needle point. The map cubes continue to own collision and navigation; the new
meshes are collisionless and do not affect nav. A lower slate `Color` value was
needed for noon lighting: the face at pixel (330, 600) sampled RGB
(187, 184, 174) at tint (0.055, 0.058, 0.060), then (146, 142, 131) after
reducing it to (0.015, 0.017, 0.018). The final close capture also samples
(137, 133, 119) on the third stone. This is visibly more restrained than the
near-ivory frame, but remains a smooth, untextured blockout surface rather than
finished stone.

The UE 5.8.3 scratch editor build succeeded (five actions for the tint change,
then four for the chipped cap), and
`CaptiveSky2.Agent.ListeningStonePresentation` passed after both changes. The
last real-RHI Game run captured a warmed noon frame at
[`003_Stone_Presentation_Close.png`](../../Saved/CompileScratch/Claude_Props/Saved/GameShots/Codex_StandingStoneChippedCap_20261009/003_Stone_Presentation_Close.png).
It disabled resident thinking and Python, used isolated world data, and ended
normally at its 60.0-second real-time cap with zero model requests. The frame
confirms the cap and tint render, not final art quality: the close camera makes
the smooth stones dominate, so texture/lichen, scale and ordinary gameplay
framing remain open for another art pass.

Logs: [`Codex_StandingStoneChippedCap_Build_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_StandingStoneChippedCap_Build_20261009.log),
[`Codex_StandingStoneChippedCap_Automation_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_StandingStoneChippedCap_Automation_20261009.log),
and [`Codex_StandingStoneChippedCap_Game_20261009.log`](../../Saved/Logs/Codex_StandingStoneChippedCap_Game_20261009.log).

## Rejected procedural lichen overlay (2026-10-09)

A collisionless second procedural-mesh section was tested as a quick lichen
surface. The focused `ListeningStonePresentation` automation passed, but four
matched real-RHI noon frames showed dark, occasionally blue-black flecks rather
than natural moss. Raising the green tint and spreading marks across every
side face did not fix the appearance. The code was removed; the current runtime
stones remain the simpler smooth, collisionless forms, and no map or Content
asset changed. This confirms the missing quality is a real textured surface,
not additional decal geometry. Diagnostic frame:
[`004_Stone_Presentation_Close.png`](../../Saved/CompileScratch/Claude_Props/Saved/GameShots/Codex_ListeningStoneLichen_bright_20261009/004_Stone_Presentation_Close.png);
log: [`Codex_ListeningStones_Lichen_bright_Game_20261009.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_ListeningStones_Lichen_bright_Game_20261009.log).

Next, test a genuine rock/lichen PBR material on the transient meshes under a
matched noon view, keeping the existing proxy collision and navigation. Do not
reintroduce flat colored patches unless a render proves they read as surface
growth at ordinary gameplay distance.

## Candidate asset check (2026-10-09)

The library contains a Tripo `StoneCairn` material and its color, normal, and
ORM textures, as well as several Fab rock/moss surface texture sets. The Tripo
cairn mesh was previously rejected for these narrow, tall proxies because its
silhouette is too broad; its material is not yet proven to suit the procedural
stone UVs. The Fab sets are texture assets, not ready-to-assign materials.
Therefore no existing surface is a verified drop-in: the next material pass
must check texture projection/tiling on the actual transient mesh and judge a
matched in-game render before keeping it. No Content asset was changed in this
audit.

**Local candidate discovered after the render audit (2026-10-09):** the project
also contains `/Game/Materials/M_StandingStoneRockSurface` as a 15,733-byte
material asset, last written at 17:52 local time. The current presentation code
does not reference that path; it still loads Engine `BasicShapeMaterial` and
sets only its `Color` parameter. The local material's graph, dependencies, UV
response, and appearance have not been inspected, so its name is not evidence
that it solves the texture problem. Next, inspect it read-only and, if its
inputs suit the procedural UVs, test it transiently in the same noon and shadow
views. Keep it out of `Content/` edits and do not replace the current surface
until a matched real-RHI capture shows a genuine improvement. No runtime code
or binary asset was changed in this follow-up.

## Local PBR surface and calibrated transient tint (2026-10-09)

The candidate is a real texture-driven rocky surface: its package references
base-color, normal, and ORM textures from the local Fab `Rocky_Ground_wfjjecl`
set, and UE reports one vector parameter named `Color`. The Game-only
Listening Stones now load that material and set its `Color` (plus the common
`BaseColor` compatibility name). If the local material cannot load, the
transient replacement is skipped and the saved map proxies remain visible.
No `.uasset`, map, collision, or navigation data was changed.

The first PBR capture exposed an overly dark result and a prominent vertical
artifact. A matched sample at pixel (830, 650), same camera and noon hour, was
RGB (111, 103, 89), compared with (161, 152, 133) on the earlier smooth-slate
surface. Raising the per-stone linear tint to roughly (0.12–0.155) brought the
same point to (148, 133, 111): a textured, weathered warm-gray stone without
the original pale, smooth blockout look. The procedural UV seam also now has a
duplicated position at U=1 on each ring, preventing the closing face from
interpolating across the full 0-to-1 texture range. Darker flecks and streaks
remain in the material's appearance and should be judged in the ordinary
landmark approach, not mistaken for a final lichen pass.

Verification: UE 5.8.3 scratch editor build succeeded; the focused
`CaptiveSky2.Agent.ListeningStonePresentation` automation passed, including
candidate loading, the material's actual `Color` parameter, position-matched
0/1 UV seams, collisionless replacement meshes, and unchanged proxy collision
and navigation. The real-RHI Game run used the matched close camera at noon,
disabled resident thinking and Python, allowed zero model requests, and exited
normally at its 60-second real-time cap. Warm frame:
[`003_Stone_PBR__Warm40.png`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/GameShots/Codex_StandingStonePBR_Balanced_20261009/003_Stone_PBR__Warm40.png).
Logs:
[`Codex_StandingStonePBR_FinalCheck_Automation_20261009.log`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_StandingStonePBR_FinalCheck_Automation_20261009.log),
[`Codex_StandingStonePBR_Balanced_Game_20261009.log`](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_StandingStonePBR_Balanced_Game_20261009.log).
The source references the local material path; since `Content/` is not tracked,
the surface asset itself remains machine-local and the saved proxies are the
safe fallback on a checkout without it.

## Transient lichen weathering (2026-10-10)

Each runtime standing stone now receives eight deterministic, irregular lichen
islands around its full circumference. The patches sample the exact triangles
of the generated rock mesh, sit just proud of the surface, and reuse the local
PBR material with a subdued olive tint. A fixed close real-D3D12 frame shows
one patch reading on the near stone; at this distance it adds a small natural
color break without covering the rock texture. Ordinary approach-distance
readability is still unverified, so patch size should not be increased until a
journey-view comparison is made.

The change is Game/PIE-only and visual-only. The original map proxies still own
collision and navigation; no map, Content asset, or persistent world state was
changed. The UE 5.8.3 editor build succeeded and
`CaptiveSky2.Agent.ListeningStonePresentation` passed under NullRHI, with
assertions for the eight-patch geometry, the transient material tint, bounds,
collisionless presentation meshes, and unchanged proxy collision/navigation.
A separate D3D12 spectator capture used an isolated data root, disabled
resident thinking and Python, allowed zero model requests, and exited normally
after 35.3 real seconds. Frame: [`001_Lichen__Close.png`](../../Playtests/Codex_StoneLichen_20261010_final/ScreenshotsCoverage/001_Lichen__Close.png).
Automation and Game logs:
[`StoneLichenAutomation_Coverage.log`](../../Playtests/Codex_StoneLichen_20261010_final/StoneLichenAutomation_Coverage.log),
[`StoneLichenGame_Coverage.log`](../../Playtests/Codex_StoneLichen_20261010_final/StoneLichenGame_Coverage.log).
