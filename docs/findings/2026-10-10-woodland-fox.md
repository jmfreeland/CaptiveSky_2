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

## Woodland-cover site selection and visual check (2026-10-10)

The first real-D3D12 frame showed the fox in an open clearing. The original
eight-point 14.5 m spawn ring had no mature spruce within 9 m, so it could only
select its first grounded fallback. The selector now samples three concentric
rings (14.5, 19, and 23.5 m from the Wind Arch), with sixteen deterministic
ground traces per ring. Eligible points are 3–14 m from a mature spruce (at
least 9 m tall); the score prefers 8.5 m of tree cover and a naturally sparse
grass pocket (fewest grass instances within 1.8 m, with each instance weighted
against tree-distance error). Ties keep the nearer ring first. The 14.5 m
minimum remains, so the fox stays outside the immediate landmark approach. If
no cover qualifies, the first valid grounded point remains the fallback.

The actual-project UE 5.8.3 build succeeded and `CaptiveSky2.Agent.GroundCover`
passed. A bounded NullRHI Island run selected `(-102709, 98102, 2638)` cm, 580
cm from mature spruce. A follow-up real-D3D12 run with the sparse-pocket score
selected `(-100915, 97968, 2869)` cm, 815 cm from mature spruce, with 43 grass
instances inside the 1.8 m pocket. Four elevated noon views show the resting
fox clearly in a natural ground-cover opening, with mature woodland nearby; the
first two frames are clearest at 1600x900. The low 17:00 test angles failed to
read the fox, so lighting and camera elevation matter as much as proximity to
trees. The rendered run used isolated world data, disabled thinking, allowed
zero model requests, and ended itself at 90.3 real seconds. This verifies
rendered presence and the request/time safeguards.

A separate 200-frame CSV profile at the same 1600x900 real-D3D12 Game settings
averaged 13.63 ms (73.4 FPS); P95 was 15.09 ms (66.3 FPS equivalent), with one
65.45 ms spike. It clears the 30-FPS floor on this RTX 4080 Laptop GPU for this
short windowed spectator sample. It does not establish PIE performance, other
hardware, or long-session stability. The profile used the same isolated data,
thinking-disabled, zero-request, 90-second safeguards.

Evidence: [`GroundCover automation`](../../Saved/Playtests/Codex_FoxCover_SparseSelector_20261010/GroundCover.log), [`bounded placement run`](../../Saved/Playtests/Codex_FoxWoodlandCover_Verify_20261010/FoxGame.log), [`real-D3D12 screenshot`](../../Saved/Playtests/Codex_FoxCover_SparsePocket_Retry_20261010/Screenshots/001_Fox__Woodland_Edge.png), [`real-D3D12 run log`](../../Saved/Playtests/Codex_FoxCover_SparsePocket_Retry_20261010/FoxRealRHI.log). The initial lower-angle comparison set is under [`Codex_FoxCover_RealRHI_20261010`](../../Saved/Playtests/Codex_FoxCover_RealRHI_20261010/Screenshots/).

CSV evidence: [`profile log`](../../Saved/Playtests/Codex_FoxCover_FrameTime_20261010/FoxFrameTime.log), [`200-frame CSV`](../../Saved/Profiling/CSV/Profile(20261010_032057).csv).

Next, repeat the matched profile in PIE and on the user's target hardware before
treating the 30-FPS floor as a platform-wide guarantee. Keep wildlife transient
and bounded; consider another species only after performance is confirmed.
