# Woodland fox: bounded woodland ecology (2026-10-10)

One wild fox now shares the Wind Arch woodland edge with the existing stag.
`AIslandWeather` creates it as transient ecology, approximately 14.5 m from the
landmark on the nearest valid landscape sample. The fox rests by day and wakes
at night. Its actor-driven movement uses walk/run clips with root motion
disabled, checks actual ground under every step, and remains inside a 6.5 m
home radius. No collision, navigation influence, memory directory, persistent
record, model call, or new asset import is added.

Residents receive one nearby species-level observation in their existing
situation summary. `WoodlandFox` is a distinct optional interaction, never a
movement destination. At close range, an awake fox looks briefly and trots
toward cover; observing a resting fox does not wake or startle it. This is
lightweight wildlife, not a conscious character or companion.

## Verification

- UE 5.8.3 editor target built successfully in the isolated
  `Saved/NavBoundsTest/ProjectCurrent` scratch project.
- `CaptiveSky2.Agent.WoodlandFox` passed, including imported mesh/animation
  skeleton compatibility, disabled root motion and collision, target tagging,
  movement-target rejection, resident perception, resting observation, wake,
  and an awake response bounded within the fox's home patch.
- A normal `/Game/Maps/Island` Game run spawned the fox at
  `(-101434, 101634, 2609)` cm by Wind Arch. It used NullRHI, disabled agent
  thinking, an isolated world-data root, a 90-second real-time cap, and a zero
  model-request limit; it ended normally after 90.1 seconds with zero requests.
- A 1600x900 real-D3D12 Game capture shows the fox clearly at gameplay scale,
  with its resting pose grounded and not hidden by the foreground cover. The
  image also makes a composition limit visible: it rests in a relatively open
  clearing rather than tucked into the denser woodland behind. This is a
  placement/composition note, not a failed ground-contact check. The temporary
  screen-message toggle was capture-only.
- No Content or map was edited. No matched frame-time profile was captured, so
  this image does not establish the project's 30-FPS performance target.

Evidence:

- [`automation`](../../Saved/Playtests/Codex_WoodlandFox_20261010/AutomationFinal/WoodlandFoxAutomation.log)
- [`bounded Game run`](../../Saved/Playtests/Codex_WoodlandFox_20261010/Game2/WoodlandFoxGame.log)
- [`real-D3D12 screenshot`](../../Saved/Playtests/Codex_WoodlandFox_20261010/RealRHI_Close/Screenshots/004_Fox_At_Wind_Arch.png)
- [`real-D3D12 run log`](../../Saved/Playtests/Codex_WoodlandFox_20261010/RealRHI_Close/WoodlandFoxRealRHI_Close.log)

## Woodland-cover site selection (2026-10-10)

The first real-D3D12 frame showed the fox in an open clearing. The original
eight-point 14.5 m spawn ring had no mature spruce within 9 m, so it could only
select its first grounded fallback. The selector now samples three concentric
rings (14.5, 19, and 23.5 m from the Wind Arch), with sixteen deterministic
ground traces per ring. It prefers a grounded point 3–9 m from a mature spruce
(at least 9 m tall), targeting 5.2 m; ties keep the nearer ring first. The
14.5 m minimum remains, so the fox stays outside the immediate landmark
approach. If no cover qualifies, the first valid grounded point remains the
fallback.

The actual-project UE 5.8.3 build succeeded and `CaptiveSky2.Agent.GroundCover`
passed. A bounded Island Game run selected `(-102709, 98102, 2638)` cm, 580 cm
from mature spruce; this confirms the selector used live scatter data rather
than its fallback. The run used NullRHI, isolated world data, disabled agent
thinking, allowed zero model requests, and ended itself at 90.1 real seconds.
This verifies placement and the real-time/request safeguards, not rendered
composition or frame rate. The earlier real-D3D12 fox image depicts the old
open-clearing site and must not be treated as a visual check of the new one.

Evidence: [`GroundCover automation`](../../Saved/Playtests/Codex_FoxWoodlandCover_Verify_20261010/GroundCover.log), [`bounded Island Game run`](../../Saved/Playtests/Codex_FoxWoodlandCover_Verify_20261010/FoxGame.log).

Next, capture the updated site with real RHI and check the fox against the
woodland edge, path readability, and Wind Arch approach at ordinary gameplay
scale. Consider another species only after the visual and performance check;
keep wildlife transient, bounded, and non-conscious unless the project
deliberately gives an animal its own identity, memory, personality, and request
budget.
