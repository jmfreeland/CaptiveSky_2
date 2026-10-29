# Generated Birch and Hazel instanced-material repair (2026-10-29)

## Finding

The bounded runtime woodland report recorded Birch Coppice and Hazel Coppice
instances alongside warnings that their materials lacked the
`InstancedStaticMeshes` usage permutation and could fall back to Unreal's
Default Material. The generator already selected six tree species; this
rendering fallback can make some of that variety difficult to see.

The nine warned base materials were inspected in UE 5.8.3. Each is a clean,
opaque surface `Material` with exactly two graph expressions: one
`MaterialExpressionConstant3Vector` and one `MaterialExpressionConstant`.
Neither graph contains a material-function call. All nine initially had
`bUsedWithInstancedStaticMeshes=false`. This repair is limited to those nine
verified roots. Rhododendron remains excluded because its master has four
unresolved function references; do not toggle or reconstruct that graph
without source evidence.

## Repair and verification

`Scripts/Enable-GeneratedCoppiceInstancedMaterials.py` performs a complete
preflight before editing: it rejects dirty content packages, requires each
root to remain a base `Material`, and checks that its expression classes still
match the audited two-constant graph. It enables the instanced-static-mesh
usage, recompiles every changed material before beginning any save, then saves
and verifies all nine roots. The operation is idempotent.

The live UE 5.8.3 editor logged `GeneratedCoppiceInstancedUsage COMPLETE` at
2026-10-29 16:25:27. Each of the five Birch and four Hazel roots changed from
disabled to enabled, with an empty compile-error list and successful saved
state. A follow-up `ObjectTools.get_properties` read returned
`bUsedWithInstancedStaticMeshes=true` for all nine. A separate dirty-state
check returned false for all nine packages. The Island map was not edited.

The editor was restarted beforehand to apply Nanite Foliage; afterward
`r.Nanite.Foliage` remained `1`.

## Still to verify

The usage permutations now compile and are saved, but their visual/runtime
effect is not yet confirmed in PIE. The earlier preview began at night and
did not provide a controlled daylight or VRAM sample. After confirming
available graphics-memory headroom, run a short daylight PIE capture and
inspect for remaining Birch/Hazel fallback warnings and visible canopy
variety. Keep the existing 1,800-second/120-request play safeguards in force,
disable agent thinking for the rendering-only check, and stop early if the
graphics-memory warning returns. This repair does not address the separate
`M_Island_Textured_Wet` invalid texture-index warning, the remaining plant
material families, water collision, or the broad environmental-diversity
work.
