# Common sea-lavender candidate (2026-10-17)

This adds an unplaced, collisionless *Limonium vulgare*-inspired asset candidate
for a salt-meadow transition. JNCC describes Atlantic salt meadows as diverse
communities and lists common sea-lavender among species present in that habitat
([JNCC H1330](https://sac.jncc.gov.uk/habitat/H1330/)). This supports the broad
habitat choice, not a claim that the species belongs at any particular Island
coordinate.

The local Blender source builds a low grey-green basal rosette and eight airy,
forking flower stems with small five-petal lavender stars. Its fixed geometry is
deterministic and uses flat-color materials; there are no paid services,
generated textures, collision meshes, Content edits, map edits, or placement
data. Blender 5.2.2 exported `SM_CoastalSeaLavender_01.fbx` at 71 × 71 × 57 cm,
with 5,808 triangles and three material slots. Two source-contract tests pass.
The exported-FBX verifier re-imported the mesh and reported one `UVMap`, zero
collision objects and `PROBLEMS []`; its 900 × 900 preview was rendered with
Cycles on CPU (8 samples), avoiding GPU contention with the open Unreal editor.
That image reads as a low leaf rosette beneath airy branched lavender stars, but
the basal leaves are visibly faceted and the tiny flowers may disappear at
normal play distance. This is a promising structure candidate, not final art
approval.

The asset is only a procedural source candidate until exported and inspected.
Next gate: import into an isolated Unreal trial folder and inspect its material,
scale, silhouette and salt-meadow ground fit under real lighting. Only after
that should a sparse test population be considered and profiled against the
30-FPS floor. Do not add it to the main map from this candidate review alone.

## Unreal import attempt

The user editor process was absent at the time of the attempt. The documented
UE 5.8.3 `UnrealEditor-Cmd` content-only import was run with an isolated
destination (`CoastalSeaLavender_20261017`), `-NullRHI`, a project-local shader
working directory, and an explicit project-local `-abslog` path. PowerShell
reported exit code 0, but the expected Unreal log, importer report, and
destination folder were all absent. An engine-only `-version` probe also exited
0 without console text. Therefore this is **not** evidence of a successful
import, and no Content assets were created. Diagnose command execution/log
routing before retrying; keep the export and source as the authoritative
candidate meanwhile.
