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

Next, improve woodland-edge composition around the fox without moving it into
the residents' open Wind Arch approach or harming path readability. Consider
one more species only after the full runtime and performance check; keep
wildlife transient, bounded, and
non-conscious unless the project deliberately grants an animal its own
identity, memory, personality, and request budget.
