# PVE Megaplant export path

## Existing audit results

The local library contains 296 asset packages across four tree families:
Common Hazel, European Aspen, European Beech, and Norway Maple. UE 5.8.3
Asset Registry reports from 2026-10-10 provide stronger evidence than package
filenames: all 16 A/B/C/D tree variants loaded as `SkeletalMesh`; none loaded
as `StaticMesh`. The same audit loaded 12 PVE graph, growth-data, and preset
assets. The graph property is protected from the tested Editor Python wrapper.
See [`MegaplantAudit_20261010.log`](../../Saved/Logs/MegaplantAudit_20261010.log)
and [`MegaplantPVEAudit_Final_20261010.log`](../../Saved/Logs/MegaplantPVEAudit_Final_20261010.log).

The tree variants are therefore not direct candidates for the Island's current
HISM tree component. Earlier disposable probes also found bare/sparse canopies
under the project's normal renderer settings; enabling experimental Nanite
foliage exposed leaves but made the tested Maple dark and coarse. The probes
did not justify enabling Nanite foliage globally or placing these trees on the
Island. See the [Maple preview](2026-10-16-megaplant-maple-probe.md),
[instanced-skeletal probe](2026-10-13-instanced-skinned-megaplant.md), and
[rejected woodland candidate](2026-10-17-megaplant-woodland-regeneration.md).

## PVE's viable export choice

The installed UE 5.8 PVE exporter supports both static- and skeletal-mesh
outputs. Its `FPVExportParams` defaults to skeletal output; its own tooltip
states that static output has no wind animation, while skeletal output carries
bones for wind sway. The exporter implementation is editor-private and has no
Python/UFUNCTION entry point, so the completed inspection scripts cannot
invoke its export programmatically. Use the PVE editor export UI for a
disposable static-mesh candidate rather than treating the imported skeletal
package as a static mesh or attempting to call private exporter code.

## Next step and guardrails

Export one species/variant into a throwaway content folder with Static Mesh
selected, leaving the source graph, imported package, and Island map untouched.
Then verify foliage materials, leaf silhouette, LOD/Nanite behavior, and a
small instanced batch in the fixed woodland-transition composition. A static
export trades away wind; use it for a distant canopy only if its silhouette
and frame cost justify that trade. Keep skeletal output for a small hero tree
only after its wind and foliage rendering are verified. Do not increase global
scatter density or enable Nanite foliage project-wide as part of this probe.
