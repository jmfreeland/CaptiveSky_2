# Tideglass glasswort candidate

## Ecological fit

Common glasswort (*Salicornia europaea*) is an annual succulent associated
with sandy or muddy saltmarsh, including tidally inundated margins and
saltmarsh-to-dune transitions, according to the [BSBI distribution database](https://database.bsbi.org/object.php?class=MultimediaInstance&objectid=2cd4p9h.vy2x0d).
The [JNCC pioneer saltmarsh description](https://sac.jncc.gov.uk/habitat/H1310/)
places glasswort communities on sheltered intertidal mud and sand, lower marsh,
creek sides, depressions, and pans. This establishes a plausible habitat
family, not the ecological fit of any particular point on the Island.

## Asset and review

Added `Scripts/AssetPipeline/assets/tideglass_glasswort.py`: a small, open
clump of fleshy, jointed stems with a few lateral forks and restrained bronze
tips. The bronze material is a static mature-art variant; there is no seasonal
simulation. The leafless, segmented silhouette is intended to read differently
from the existing grasses, rushes, and flowering plants. It has no collision.

Blender 5.2.2 built and re-imported the FBX in an isolated ignored review root:
`Saved/CaptiveSky/ComfyBlender/Codex_TideglassGlasswort_20261017/`. Structure
verification reported `PROBLEMS []`, dimensions 34 x 34 x 28 cm, two populated
material slots, one UV layer, no collision meshes, and 1,722 triangles. A
CPU-only Cycles preview was rendered and visually inspected; the low-poly
silhouette reads as a compact branching succulent, though the stylization is
obvious.

## Gate before placement

This is an asset candidate only. It has not been imported into Unreal or placed
on the Island. Review the material colors, silhouette, scale, shadowing and
small-clump performance in a normal UE viewport or bounded Game view before
choosing a sheltered muddy/sandy margin. Do not scatter it over the pool or
claim a specific saltmarsh site without that landscape review.
