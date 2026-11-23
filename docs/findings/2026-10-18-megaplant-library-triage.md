# Megaplant library triage — 2026-10-18

## Local inventory and decision

The ignored project `Content/Megaplant_Library` currently contains four tree
species with UE skeletal-mesh variants and PVE data: Common Hazel, European
Aspen, European Beech, and Norway Maple. This is local Content, not a portable
source asset set in Git.

Existing real-RHI notes reject the current Beech A preview (bare at the
project's default Nanite Foliage setting) and the Norway Maple A preview (gray
in base-color capture and nearly black in final color, even after the
transient tint assignment). See the [Maple preview finding](2026-10-16-megaplant-maple-probe.md).
Do not scatter those two variants or enable Nanite Foliage globally based only
on the current evidence. Aspen and Hazel have not yet been rendered in the
Island lighting; their availability is not evidence that they are ready for
placement.

Useful first-variant paths for the existing disposable preview helper are:

- `/Game/Megaplant_Library/Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_A.Tree_European_Aspen_01_A`
- `/Game/Megaplant_Library/Tree_Common_Hazel/Tree_Common_Hazel_01/Tree_Common_Hazel_01_A.Tree_Common_Hazel_01_A`

The generic [transient preview helper](../../Scripts/Preview-InstancedSkinnedMegaplant.py)
already accepts an asset-path override and compares one instanced-skinned
instance with one ordinary skeletal-mesh instance. Reuse it for Aspen and Hazel
under the same Island lighting and capture settings, and retain its
zero-model-request, no-map-save behavior. Only a readable canopy with correct
material color should advance to a small, bounded runtime draw-cost test; then
review Rat-hollow canopy concealment and human-height composition before any
Island placement.

## Editor availability at triage

The main project log records `Starting Live Coding compile` at
`2026.10.16-10:13:37` on editor frame 148, with no later Live Coding completion
entry. The last log append observed here is the background DDC maintenance
entry at `2026.10.18-00:49:21`, also stamped frame 148. This is consistent with
an editor whose normal frame/tick progress stopped while a background service
continued logging; it does not prove the exact hang cause.

The Windows inventory continued to list both the project editor and Message
Log windows, but screenshot capture returned another app and activating the
project editor timed out twice. No editor was killed, restarted, or saved by
this triage. Until the editor is responsive again, do not launch a second
full-content preview beside it and do not treat any of the four tree species as
visually approved.

No `Content/` asset, saved map, canonical wildlife-home record, or model-request
setting was changed for this inventory. Build, material, wind, collision,
render-cost, and in-game concealment validation remain outstanding.

## Responsive-editor preview follow-up

On 2026-10-18, the user confirmed that Unreal Editor was open and responsive.
The Aspen A skeletal-mesh asset was then opened directly in its asset preview.
In the reference pose, the visible tree was bare branching geometry with no
readable leaf canopy. The editor also warned that it had encountered a Nanite
Assembly while Nanite Foliage was disabled, so the asset might not appear as
expected. No project setting was changed to dismiss or work around that warning.

The Common Hazel A asset was opened in the same live editor for a read-only
check. It likewise showed only bare branches in its reference pose, with the
same Nanite Assembly warning. The viewer overlay reported 389,750 triangles and
201,072 vertices; the asset tooltip reported 3,714 source triangles, 1,855
bones, one LOD, and 9,148,498 Nanite triangles. The differing mesh counters are
recorded as reported rather than reconciled; neither is a gameplay profile.

This live preview is not evidence that Aspen is intrinsically leafless: the
Nanite Assembly warning may explain the missing canopy, and a reference-pose
asset preview is not the Island's final rendered material path. It is enough to
keep the species behind the visual-approval gate. The proposed runtime scatter
was withdrawn before any build, PIE, or map save; no Megaplant instances were
added to the Island. The next useful step is a controlled isolated preview
after the Nanite Assembly/material path is understood, with no global renderer
setting change and no second full-content preview while the recorded video
memory pressure remains unresolved.
