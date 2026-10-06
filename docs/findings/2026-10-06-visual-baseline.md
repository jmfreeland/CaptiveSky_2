# Rendered Island visual baseline (2026-10-06)

Two bounded spectator passes captured the current authored viewpoints at
1600×900: one using the isolated world's default 09:00 clock, and one with the
explicit `-ViewpointHour 17` override. The second pass confirms the runtime
viewpoint reader does not apply the JSON's 17:00 `hour` field by itself.
Both sessions ended at 120.2 real seconds with zero model requests; the 17:00
clock lived only under its separate `Saved/Playtests/Codex_GoldenHourHighlights_20261006`
data root. This was a visual review, not a frame-rate measurement.

A later controlled spot-check reused the exact `WindArchOverlook` camera and
`-ViewpointGroundCover -ViewpointNoWorldState` settings, changing only the hour:
[12:00](../../Saved/Viewpoints/2026-10-06_170622_h12.0/04_WindArchOverlook.png)
and [17:00](../../Saved/Viewpoints/2026-10-06_170207_h17.0/04_WindArchOverlook.png).
The daylight frame shows that the landscape material is present and the ground
cover is legible; the evening frame's near-black foreground is chiefly the
stronger tree/arch shadow at this angle. This pair is sufficient reason not to
brighten or replace the global landscape material to solve this single-view
lighting problem. It is still an editor-world SceneCapture, not a gameplay
lighting or frame-rate test; its separate dense SceneCapture timing screen read
55.50 FPS, which is not evidence of the 30 FPS gameplay floor.

## What the images show

- The Island already has very dense, tall near-ground cover at Tideglass and the
  Cairn, but broad areas in the Shore Approach and distant Wind Arch views read
  as nearly bare. The opportunity is composition and distance layering, not a
  blanket increase to the current ground-cover population. The ground-detail
  frame also has no clearly readable walking lane through the close foliage.
- The ListeningStones/WindArch silhouette is dominated by very large,
  round-stacked blockout rocks. The intended arch is visible, but its irregular
  mass crowds the frame and competes with the inn and residents. Keep the
  established landmarks and interaction targets fixed while improving the
  visible stone forms and scale.
- The 17:00 pass produces a striking warm sky and tree silhouettes. It also
  pushes the ground and foreground into deep shadow; the golden-hour shot is a
  useful mood reference, not evidence that the current foreground lighting is
  balanced. Do not compensate by changing the global day/night cycle from this
  single pass.
- Both controlled spot-checks show the blue blockout Wind Arch. That is expected
  for this editor-world SceneCapture: `UIslandWindArchPresentationSubsystem`
  installs its transient rock replacement only in Game/PIE. The shots therefore
  cannot judge the in-game stonework presentation; use an actual PIE/Game frame
  for that decision.

## Next visual pass

Use one Tideglass-to-Wind-Arch composition to improve the mid- and far-distance
habitat layers and give the landmark a more legible natural silhouette, while
keeping the instance budget and gameplay anchors unchanged. Compare at both a
readable daylight hour and 17:00. The local shots below are the visual reference
set; their image files are generated evidence, not source-controlled assets.

Morning examples: [shore approach](../../Saved/Playtests/Codex_HighlightSet_20261006/Screenshots/001_Shore_Approach.png),
[Tideglass](../../Saved/Playtests/Codex_HighlightSet_20261006/Screenshots/005_Tideglass.png),
[Tideglass ground detail](../../Saved/Playtests/Codex_HighlightSet_20261006/Screenshots/006_Tideglass_Ground_Detail.png),
[Wind Arch](../../Saved/Playtests/Codex_HighlightSet_20261006/Screenshots/008_Wind_Arch_Overlook.png).

Golden-hour examples: [inn from path](../../Saved/Playtests/Codex_GoldenHourHighlights_20261006/Screenshots/002_Inn_From_Path.png),
[Tideglass](../../Saved/Playtests/Codex_GoldenHourHighlights_20261006/Screenshots/005_Tideglass.png),
[Listening Stones](../../Saved/Playtests/Codex_GoldenHourHighlights_20261006/Screenshots/007_Listening_Stones.png),
[Wind Arch at 17:42](../../Saved/Playtests/Codex_GoldenHourHighlights_20261006/Screenshots/008_Wind_Arch_Overlook.png).
