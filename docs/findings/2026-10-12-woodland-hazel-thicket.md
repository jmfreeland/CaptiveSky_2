# Woodland hazel thicket asset candidate (2026-10-12)

The Tideglass-to-Woodland Game frame shows a broad open midground below a thin,
repeating tree line. To explore a local, lightweight way to break up that
transition, `Scripts/AssetPipeline/assets/woodland_hazel_thicket.py` now builds
a collisionless, multi-stem deciduous shrub mass. It is intended for sparse
woodland-edge groups and potential small-animal cover, not a global increase in
foliage density.

## Asset verification

Blender 5.2.2 exported and then re-imported the FBX for its preview. After the
first review showed too much bare stem, a second pass added leaves along the
primary branches as well as the side shoots. The measured dimensions are
193.4 x 186.8 x 233.5 cm, with the origin grounded at `ZMIN=0.0213 m`. The mesh
has one `UVMap`, four parameter-only material slots, 5,778 triangles, and no
collision meshes. The verifier reports
`PROBLEMS []` / `VERIFY DONE`. The local export and preview are under
`Saved/CaptiveSky/ComfyBlender/WoodlandHazelThicket/` (gitignored).

## Unreal import and transient preview

The UE 5.8.3 commandlet imported the FBX to the fresh local review folder
`/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012/`. Its
`unreal_import.json` reports matching dimensions, all four material slots,
zero collision shapes, and `validation_errors: []`. No existing asset was
reimported. The original Island map was never saved. A two-stage UE Python
workflow made an isolated 171 MB copy at
`/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012/Maps/Island_WoodlandHazelThicketTrial_20261012`
and saved exactly one no-collision preview actor at the documented woodland
candidate location. The original `Content/Maps/Island.umap` kept its prior
modification time.

The bounds-framed preview helper produced an [isolated capture](../../Saved/Playtests/Codex_WoodlandHazelThicketPreview_20261012/WoodlandHazelThicket_RealRHI.png)
and a [full-scene capture](../../Saved/Playtests/Codex_WoodlandHazelThicketPreview_20261012/WoodlandHazelThicket_FullScene.png).
Both render leaves unusually tan/black and the central stems very dark. The
import audit and preview log confirm the expected material assignments and
connected base-color values, but those SceneCapture frames did not establish
how the mesh looks in the normal Game viewport. Similar capture-path ambiguity
occurred with the bluebell candidate, so a two-view, bounded Game trial was run
on the isolated Island copy. The trial reached world-ready in 36.1 seconds,
captured close and mid-distance frames, and exited normally after 90.1 seconds
with thinking disabled and zero model requests. In both views, the hazel's
central crown renders nearly black, sparse, and skeletal against the lit
understory; the exposed branching and flat leaf shapes do not read as a
believable shrub. This confirms the art concern is visible in Game, not just a
SceneCapture artifact. The copy emitted a `WaterEditor` import warning, but the
woodland vegetation and candidate rendered. Treat this candidate as rejected
for Island placement unless its mesh/material presentation is substantially
reworked.

Evidence: [close Game view](../../Saved/Playtests/Codex_WoodlandHazelThicketGame_20261012/Screenshots/001_Close.png),
[mid-distance Game view](../../Saved/Playtests/Codex_WoodlandHazelThicketGame_20261012/Screenshots/002_Mid_Distance.png),
[bounded-run log](../../Saved/Logs/Codex_WoodlandHazelThicketGame_20261012.log).

## Review limits and next gate

This is only a stylized source-mesh candidate. The Game review confirms it
reads as an unnatural black, skeletal thicket at both close and mid-range. The
Unreal MCP bridge remains unavailable. HISM draw cost is untested. The
candidate exists only in the isolated review folder/map and has not been placed
on the Island. Do not promote it based on the structurally valid FBX or Blender
preview. Next, build a substantially different, fuller woodland-edge species
using foliage forms/materials already proven in Game, then compare a single
candidate and a sparse group against existing cover at the same view. Keep the
brown midground composition problem separate from a global foliage-count bump.
