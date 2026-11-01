# Live editor view of the woodland fungus patch — 2026-11-01

The connected UE 5.8 editor is on `/Game/Maps/Island`. Clearing the stale
Outliner filter and framing the existing `WoodlandFungusCluster_Review_01`
actor revealed the saved close-up placement in the live level. The fungus
cluster reads as five pale ochre caps with light stems at close range. Dense
grass crosses some lower stems, while the immediate ground remains visibly
brown and the wider slope is sparse. This supports the earlier observation
that the placement is present, but does not establish normal player-distance
readability or a polished woodland composition.

The actor was selected only to frame the editor camera, then deselected. The
viewport navigation and Outliner filter are transient editor state; no actor
transform, component, package, or map was edited or saved. This is an editor
view, not a PIE/Game capture, and the foliage shown is not proof of the
worktree's newer scatter module running in this separate project.

Next, compare the same cluster from a slightly elevated visitor-scale view
after a safe runtime/editor validation session is available. Check whether
grass occlusion feels natural and whether the ochre material retains shape
without looking pale in direct light. Keep the current count and placement
until that comparison; do not add more fungus instances based on this close
frame alone. Rendered play remains subject to the GPU and system-memory gates.
