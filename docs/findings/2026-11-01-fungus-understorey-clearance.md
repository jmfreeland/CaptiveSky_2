# Woodland fungus understorey clearance — 2026-11-01

The close live-editor view showed dense grass crossing the lower stems of the
five-mushroom woodland cluster. Ground-cover scatter now recognizes saved mesh
components using `SM_WoodlandFungusCluster_01` and skips candidates whose
combined cluster/foliage footprints would overlap, with a 25 cm breathing
margin. The clearance follows each placed component's world-space bounds and
each selected ground-cover mesh's scaled horizontal bounds. It changes no
candidate budget, adds no instances, and does not alter the authored fungus
count or placement. Other plants remain eligible immediately beyond the
measured visual footprints, so the patch is not replaced by a large circular
bald spot.

`CaptiveSky2.Agent.FungusGroundCoverClearance` passed in the isolated UE 5.8.3
scratch project under NullRHI. It checks the combined-footprint boundary,
2D grounding independence, and that a non-finite footprint cannot create an
unbounded clearing. The UE 5.8.3 editor target build also succeeded. Test log:
`Saved/CC/Project/Saved/Logs/CaptiveSky_2.log`.

This validates the clearance calculation and compilation, not the live scatter
integration or normal-distance appearance. The open D: editor and Island map
were left untouched; no PIE/Game session was started because the live GPU
headroom gate remains in force. Next, after a safe rendered session is
available, compare the fungus from visitor scale and check that the small
clearings read as natural woodland floor rather than isolated bare rings.
