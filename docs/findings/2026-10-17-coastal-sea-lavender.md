# Common sea-lavender candidate (2026-10-17)

This adds an unplaced, collisionless *Limonium vulgare*-inspired asset candidate
for a salt-meadow transition. JNCC describes Atlantic salt meadows as diverse
communities and lists common sea-lavender among species present in that habitat
([JNCC H1330](https://sac.jncc.gov.uk/habitat/H1330/)). This supports the broad
habitat choice, not a claim that the species belongs at any particular Island
coordinate.

The local Blender source builds a low grey-green basal rosette and eight airy,
forking flower stems with small five-petal lavender stars. Its fixed geometry is
deterministic and uses flat-color materials; there are no paid services or
generated textures. Blender 5.2.2 exported `SM_CoastalSeaLavender_01.fbx` at 71 × 71 × 57 cm,
with 5,808 triangles and three material slots. Two source-contract tests pass.
The exported-FBX verifier re-imported the mesh and reported one `UVMap`, zero
collision objects and `PROBLEMS []`; its 900 × 900 preview was rendered with
Cycles on CPU (8 samples), avoiding GPU contention with the open Unreal editor.
That image reads as a low leaf rosette beneath airy branched lavender stars, but
the basal leaves are visibly faceted and the tiny flowers may disappear at
normal play distance. This is a promising structure candidate, not final art
approval.

The procedural source and local trial import are review candidates, not final
art approval. The isolated Unreal captures below show that its current material
and fine-stem presentation is not ready for map placement. Next gate: improve
the stem/flower readability or choose a better-fit asset, then review a sparse
population against the project's 30-FPS floor. Do not add it to the main map
from this candidate review alone.

## Unreal trial import

The initial sandboxed command attempts produced no report or destination
assets. Re-running the documented UE 5.8.3 content-only import with approved
access to engine log/temp paths succeeded (process exit 0). The local trial
folder is `/Game/Generated/ComfyBlender/CoastalSeaLavender_20261017/` and
contains one StaticMesh plus three flat-color materials. Its report records
71.1 × 71.4 × 57.2 cm, all three material slots, zero collision shapes as
intended, and `validation_errors: []`; Unreal's content validators processed
all four imported assets. See
`Saved/CaptiveSky/ComfyBlender/CoastalSeaLavender/unreal_import.json` and
`Saved/Logs/Codex_SeaLavenderImport_20261017_Elevated.log`.

The importer did not save a level. Unreal's startup log does show the default
`/Game/Maps/Island` map being loaded while the commandlet initialized, so this
was not a map-free process; no Island level save appears in the log, and the
only reported package saves are the four trial assets.

## Isolated Unreal shading review

Two real-RHI previews used the non-saving `Preview-GeneratedStaticMesh.py`
harness in `isolated` mode beside `ListeningStones`, so no landscape or
population obscured the mesh. The original material capture is
`Saved/Playtests/Codex_SeaLavenderUnrealPreview_20261017/SeaLavender_Isolated_RealRHI.png`.
The broad basal leaves render as pale, hard facets, while many fine upright
stems/branches read as black lines and the tiny flowers are difficult to pick
out. The preceding full-scene capture is
`Saved/Playtests/Codex_SeaLavenderUnrealPreview_20261017/SeaLavender_RealRHI.png`;
the isolated view confirms the problem is intrinsic to this candidate rather
than only background clutter.

As a narrow A/B, the same source was imported into
`/Game/Generated/ComfyBlender/CoastalSeaLavender_TwoSided_20261017/` with only
leaf and flower materials marked `TwoSided: true`. The Unreal import report
confirmed those properties (`true`, with stem remaining `false`), correct
material assignments, intended zero collision, and no validation errors. Its
isolated real-RHI capture is
`Saved/Playtests/Codex_SeaLavenderUnrealPreview_20261017/SeaLavender_TwoSided_Isolated_RealRHI.png`.
Visual review finds no material improvement in the pale leaves, dark stem
structure, or flower readability.
Two-sided shading is therefore not retained as a candidate-specific fix; the
importer keeps it as an optional manifest property for other assets. These
captures are a transient review only: the script spawned an actor in the
loaded Island world but never called a level-save API. No placement or
gameplay/population cost is approved.

## Source readability iteration (2026-10-17)

In response to the real-RHI result, the source now uses a darker sage basal
rosette, thicker/brighter stem tubes, larger lavender stars, and fewer
branch/flower elements. The source contract remains deterministic and
collisionless. Blender 5.2.2 rebuilt this candidate into the isolated
`Saved/Playtests/Codex_SeaLavenderReadabilityTrial_20261017/CoastalSeaLavender/`
output, leaving the original `Saved/CaptiveSky/ComfyBlender/` export and both
Unreal trial imports untouched. The exported mesh is 68 × 72 × 57 cm with
4,528 triangles (down from 5,808), one UV layer, three material slots, no
colliders, and verifier `PROBLEMS []`; all three source tests pass. The
[Blender preview](../../Saved/Playtests/Codex_SeaLavenderReadabilityTrial_20261017/CoastalSeaLavender/Preview.png)
shows distinct pale-purple florets and export-safe stems, but Blender shading
does not predict the earlier Unreal lighting failure. This is an improved
source candidate, not approval for import or Island placement.

Next: when the UE preview path is available, import this exact FBX to a fresh
disposable destination and repeat the same isolated real-RHI capture beside
the baseline. Compare stem visibility, leaf color, normal play distance,
shadows, and a 5–10-clump frame-time sample before considering any shoreline
population.
