# Tideglass rock-samphire candidate — 2026-10-17

Added a deterministic Blender asset recipe for a low, fleshy rock-samphire
(Crithmum maritimum) clump at Tideglass. Its five clustered scapes, branching
blue-green foliage and small muted-yellow umbels provide a different silhouette
and palette from the existing narrow-leaved, pink-flowered sea-thrift
candidate. The source produces three flat-color material groups, uses no
generated textures or randomness, and adds no collision or navigation
geometry.

All three source-contract tests pass. Blender 5.2.2 exported a 4,410-triangle
FBX with 27 × 47 × 40 cm bounds, three material slots and no collision. The
pipeline verifier re-imported it with one UV map and reported `PROBLEMS []`;
the CPU preview is at
`Saved/CaptiveSky/ComfyBlender/TideglassRockSamphire/Preview.png`.

The generated FBX remains a local candidate; it has not been imported into the
Unreal project or placed in the saved Island. The user's Unreal Editor PID
22688 is being left open, so the Blender preview is not an Unreal lighting,
scale, gameplay or composition approval. Next, when an editor session is
available, import beside the Tideglass sea-thrift candidate and judge both
against the actual shoreline before deciding whether to add a small, sparse
wet-edge grouping.
