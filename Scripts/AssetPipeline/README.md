# Asset pipeline: ComfyUI textures + scripted Blender -> Unreal-ready props

Local, no paid credits. ComfyUI generates tileable surface textures, `pbr.py` derives PBR maps, and headless Blender builds,
bakes and exports each prop from a small script. **The asset scripts in `assets/` are the source of truth**: to change a prop, edit
its script and rerun (10-60 s). Exports land in `Saved/CaptiveSky/ComfyBlender/<Asset>/`, which is gitignored (outputs and textures are
local; only these scripts are in git). Same review-then-import idea as `Saved/CaptiveSky/Tripo/`, but geometry and textures are
authored here instead of generated as one mesh. Nothing here touches `Content/`.

The 21 scripted props made so far (index with sizes/tris/materials, on disk): `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.

## Layout

```
Scripts/AssetPipeline/
  config.py          every path (repo-relative; env overrides). Read this first.
  make.py            orchestrator: build+bake+export -> pack textures -> verify by re-import + render
  lib.py             Blender-side toolkit (primitives, UV, materials, stone bake, collision, FBX, manifest)
  run_asset.py       Blender entry: loads assets/<name>.py, builds, finalizes
  pack_asset.py      raw stone bakes -> Unreal textures (normal flip, ORM pack)
  verify_asset.py    re-imports the exported FBX, rebuilds materials from exported PNGs, renders Preview.png
  pbr.py             albedo -> normal/roughness/AO/ORM (tileable)
  pack_library.py    add a texture to the shared library
  assets/*.py        one script per prop  <- edit these
  workflows/*.json   ComfyUI workflows used for the textures (seeds included)
  comfy_custom_node/ the SeamlessTile ComfyUI node (copy into ComfyUI/custom_nodes/)
  legacy_well/       scripts from the hand-built well (see "The well" below)

Saved/CaptiveSky/ComfyBlender/            (gitignored, local)
  ASSETS.md  Library/Textures/  Library/Sources/{stone_face,moss}/  _work/  <Asset>/{SM_<Asset>_01.fbx,manifest.json,Preview.png,Textures/}
```

Every FBX: centimetres, Z up, origin at ground centre, one UV channel `UVMap`, normals exported, tangents on, `UCX_` (convex) / `UBX_`
(box) collision embedded. `Preview.png` is rendered **from the exported FBX and PNGs only**, so it shows what the import will contain.

## Setup (once)

| Piece | Notes |
|---|---|
| ComfyUI + comfy-cli | ComfyUI at `C:\Users\freel\Documents\comfy\ComfyUI`; comfy-cli 1.22 is a global uv tool. Checkpoint `sd_xl_base_1.0.safetensors` in `models/checkpoints` |
| Python venv | `D:\python\3d` (Python 3.12, torch cu128, ComfyUI requirements, `Pillow numpy scipy`). Python 3.14 venvs were empty/risky |
| Custom node | copy `comfy_custom_node/seamless_tile/` to `ComfyUI/custom_nodes/` |
| Blender | 5.2 LTS, run headless; set `BLENDER_EXE` if it is not under Program Files. No addon/MCP needed |
| Source maps | the stone bake reads `Library/Sources/{stone_face,moss}/{albedo,roughness,normal_gl}.png` (made with `pbr.py`, see below) |
| GPU | RTX 4080 Laptop 12 GB: ComfyUI and Cycles baking compete for VRAM, so stop ComfyUI before baking |

```powershell
D:\python\3d\Scripts\activate
comfy --workspace C:\Users\freel\Documents\comfy\ComfyUI launch --background   # http://127.0.0.1:8188
comfy --workspace C:\Users\freel\Documents\comfy\ComfyUI stop
```

## Make or change a prop

```powershell
python -I Scripts\AssetPipeline\make.py birdbath wheelbarrow      # names of assets/<name>.py
```

1. Edit/add `assets/<name>.py` (API below). 2. Run `make.py`. 3. Open `<Asset>/Preview.png`; fix what looks wrong; rerun.
It prints dimensions, slots, `ZMIN` (ground contact) and `PROBLEMS []`. Logs are in `_work/logs/`. Rebuilds are deterministic: the bird bath,
wheelbarrow and rune stone rebuilt from this repo copy matched the earlier size and triangle counts exactly.

```python
NAME = "BirdBath"; BAKE_SIZE = 2048            # NAME = export folder; mesh is SM_<NAME>_01
def build(A):
    A.moss = (0.7, 0.42)                       # optional: (damp height m, moss threshold; higher = less moss)
    A.lathe("stone", [(r, z), ...], segs=40, tile=1.2)            # revolve a closed profile about Z
    A.box(mat, (sx, sy, sz), loc, rot_deg, bevel=0.01, grain="X|Y|Z")
    A.cyl(mat, r, h, loc, rot_deg, r2=None, segs=24, uv="cyl|box")
    A.tube(mat, [Vector, ...], r, segs=8)                         # pipe / rope / handle along a path
    A.rock(mat, (rx, ry, rz), loc, rot_deg, seed, sub=2, rough=0.14)
    A.collide_box(size, loc, rot); A.collide_cyl(r, h, loc)       # UBX_ / UCX_ meshes
```

Metres, Z up, origin at ground centre. Material keys: library `wood clay straw strawcoil parchment` (tiled, shared textures), `stone`
(stone-face + moss shader, **baked per asset** to a unique atlas), and plain `iron rope char ash water brass leather dirt ember rune lanternglow`
(parameters only; the emissive ones carry `Emissive`/`EmissiveStrength` in the manifest). Wood grain follows the part's local Z unless `grain=` says otherwise.
See `assets/*.py` for working examples (`runestone.py` places emissive rune geometry by raycasting onto the stone).

## Textures

Workflow = SDXL -> `SeamlessTile` -> CLIPTextEncode x2 -> KSampler (dpmpp_2m / karras, 25 steps, cfg 7) -> VAEDecode -> SaveImage, 1024x1024, batch of 4
(`workflows/*.json`). Run, pick from a contact sheet, check tiling by laying the image 2x2:

```bash
comfy --json run --workflow workflows/<name>.json --where local --wait | tee run.json
comfy --json download --out-dir outputs/cand_<name> < run.json
python -I Scripts/AssetPipeline/pbr.py <albedo.png> <out_dir> --normal-strength 3.0   # maps; tiling preserved
python -I Scripts/AssetPipeline/pack_library.py Clay <out_dir>                        # -> Library/Textures/T_Lib_Clay_*
```

Then add the key to `TILED` in `lib.py` (texture base name, tile size in metres). **Keep each material its own texture**: stone, moss, wood, clay,
straw are separate and moss is blended in the shader (one "mossy stone" prompt gave a flat green reptile-skin look). Shared negatives:
`cartoon, illustration, painting, 3d render, perspective, horizon, vignette, border, frame, text, watermark, strong shadows, blurry, low quality, oversaturated`.

| Texture | Positive prompt (all start `seamless tileable texture,`) | Extra negatives |
|---|---|---|
| Wood | `weathered wooden planks, vertical grain, aged grey-brown driftwood, flat orthographic front view, even diffuse lighting, no shadows, highly detailed, photographic PBR albedo` | |
| Stone blocks | `rough-cut grey limestone block wall, running bond masonry, recessed mortar joints, weathered natural stone, subtle colour variation, flat even lighting, photographic, orthographic straight-on view, high detail` | moss, green, grass, plants |
| Stone face (no joints) | `flat weathered limestone surface, fine natural grain, small pits and pores, subtle warm grey colour variation, macro close-up, flat even lighting, photographic, high detail` | joints, mortar, brick, blocks, cracks, lines, moss, green |
| Moss | `dense natural moss carpet, macro close-up, soft clumps of green moss, photographic, flat even lighting, top-down view, high detail` | stones, rocks, flowers |
| Clay | `terracotta clay pottery surface, slightly rough fired earthenware, warm orange-brown, subtle uneven glaze, fine scratches, macro close-up, flat even lighting, photographic, high detail` | pattern, tiles, grid |
| Straw | `dry straw thatch bundle, golden yellow straw stalks, tightly packed, macro close-up, flat even lighting, photographic, high detail` | green, grass, field, flowers |
| Parchment | `aged parchment paper, cream beige, subtle fibres and faint stains, flat even lighting, photographic, high detail` | lines, folds, creases, border, hole, writing |

Library tile sizes: wood 1.0 m, clay 0.6, straw 0.5 (long stalks), strawcoil 0.4 (twisted coils), parchment 0.5. `pbr.py --normal-strength` used:
stone face 7, wood 3, moss 3, clay 3, straw 4, parchment 0.7 (2.0 looked like popcorn). Library textures are DirectX-normal, ORM = R AO / G roughness / B metallic.

To regenerate the stone/moss source maps: run `stone_face.json` / `moss_carpet.json`, pick, run `pbr.py`, and copy `albedo.png roughness.png normal_gl.png`
into `Library/Sources/stone_face/` and `.../moss/`. Changing a library texture updates every asset using it on rebuild; stone assets need a rebake (the rebuild does this).

## Import into Unreal

Use [`docs/asset-import-pipeline.md`](../../docs/asset-import-pipeline.md) and `Scripts/Import-ComfyBlenderAsset.py` to import one local pack into `/Game/Generated/ComfyBlender/<Asset>/` without editing the Island map. NoticeBoard and StoneLantern have been imported; their bounds, material assignments, and embedded colliders match their manifests in Unreal. Their rendered appearance has not yet been visually reviewed. `Content/` is gitignored and local-only; inspect imported assets in Unreal before map placement.

## The well

The well is now reproducible from `assets/well.py`, like the other current props. `legacy_well/` preserves the earlier one-off bake/export utilities as history; edit and rebuild `assets/well.py` for current changes.

## Gotchas hit so far

- **comfy-cli is a global uv tool**; it created an empty `.venv` inside the ComfyUI folder that crashed on `import yaml`. Use one real venv with ComfyUI's
  requirements. The PyPI package behind `import yaml` is `PyYAML`.
- **Blender MCP, port 9876.** Claude Desktop's "blender-mcp" extension and the `mcp-for-blender` addon both use 9876 with different protocols
  (errors: "Incomplete JSON response", then WinError 10053). The pipeline runs Blender headless and does not need either; `--factory-startup` also stops
  the MCP addon from starting a server that fights an open Blender.
- **bmesh-created UV layers are named `Float2` in Blender 5.2**, not `UVMap`. A shader `UV Map` node reading "UVMap" then finds nothing and every texture
  samples one constant texel (flat colours, bogus normals, garbage AO) while a live Eevee render looks fine. `lib.py` creates the layer as `UVMap`
  and asserts it before every bake. Only re-importing the exported FBX caught this: do not skip verification.
- **Moss dampness is relative to height.** "Wet below 70 cm" turns a 25 cm campfire stone into a green bush. Use `A.moss`.
- Boolean modifiers leave an empty material slot 0 (faces render default white). Cube projection on a cylinder shows seams: use `uv="cyl"` (integer tile count
  round the circumference) or real block geometry with a joint-free texture. Geometry `Position` in a shader is world space.
- `Mix` nodes have several same-named sockets (float / vector / colour): pick the enabled one. When replacing a shader link, don't remove every link on the output.
- Cycles baking needs the shader to sample its tiled textures through an explicit `UV Map` node on the original UV; the atlas is only the bake target.
- Rotation signs: a box rotated about Y by +theta lifts its -X end (the first wheelbarrow had dipping handles). Look at the previews.
- Emissive materials render near-white in Eevee; they will look different under Unreal's exposure.
- Big Python files through a bash heredoc can break the shell tool: write files with the file tool.

## Limits (2026-10-10)

- NoticeBoard and StoneLantern imports' scale, material assignments, and embedded collision were checked in Unreal; neither has had an in-editor visual review or map placement. Other generated props remain unimported/unverified.
- Moss/dampness/per-block variation are baked: no runtime wetness or moss controls. Library textures tile uniformly (planks and straw repeat). Water is a static disc.
- Collision is coarse (cylinders/boxes). PBR maps are heuristics from one colour image. Meshes are 660 to 8,414 triangles: not tuned for LODs or Nanite settings.
