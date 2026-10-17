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
art approval. Next gate: inspect the imported mesh under real Unreal lighting,
then check a deliberately sparse population against the project's 30-FPS floor.
Do not add it to the main map from this candidate review alone.

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
only reported package saves are the four trial assets. Unreal shading and
in-world appearance remain unreviewed.
