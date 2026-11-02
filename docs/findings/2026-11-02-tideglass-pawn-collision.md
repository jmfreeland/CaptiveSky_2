# Stop the Tideglass blockout surface from supporting the player

## Finding

The Tideglass subsystem replaces the flattened blockout sphere visually with
a transient procedural water surface and hides the sphere in game. The
replacement water surface intentionally has no collision, but the hidden
blockout sphere retained its authored Pawn-blocking response. The previous
regression only checked that the new visual had no collision; it did not
check whether the hidden source still supported the visitor. This is a
source-level explanation for the reported ability to walk across the pool,
but rendered traversal has not yet confirmed the diagnosis.

## Change and regression

At world begin, the subsystem now saves the blockout component's Pawn response
and changes only `ECC_Pawn` to ignore—even if the visual water material is
missing and the white blockout remains visible. When transient water is
available, the same response stays active while the blockout is hidden and the
procedural surface is shown. Query/physics collision remains enabled for other
uses, and other channel responses remain unchanged. `RestorePoolMaterial`
restores the authored Pawn response before restoring visibility, including
during teardown.

`CaptiveSky2.Agent.IslandTideglassSurface` now starts with a QueryAndPhysics
pool sphere that blocks Pawn and overlaps Visibility. It checks the visible,
non-blocking fallback before any material is applied, then checks that active
water keeps the same Pawn/query behavior and finally restores the original
Pawn block and other channel response during teardown.

The isolated UE 5.8.3 `CaptiveSky_2Editor` target compiled successfully,
including the subsystem and focused regression (4 incremental build actions
after the final assertion). The automation and player traversal were not run:
the known Unreal startup instability remains, and the open D: editor belongs
to a separate project checkout. Treat the fix as compile-checked, not yet
runtime-verified. Next validate the focused automation and walk into and out
of Tideglass in a safe, matching Game/PIE build.
