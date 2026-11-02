# Live Island viewport: exposed-ground composition — 2026-11-02

## Observation

The open UE 5.8.3 editor shows the Island editor viewport at ground level near a
very large foreground rock. In the visible slice, groundcover is present in
scattered tufts and patches, but substantial brown substrate remains exposed;
the rock occupies much of the frame, and no mature tree canopy is visible in
this particular view. This matches the earlier concern that local plant life
does not yet make every visitor-facing scene feel lush or composed.

This is a point-in-time editor viewport observation, not a saved screenshot,
runtime capture, or Island-wide vegetation census. The camera framing hides
most of the surrounding landscape, so it cannot establish tree-species
richness, overall scatter quality, or player-distance readability. It also
provides no FPS, VRAM, or texture-budget measurement. No level or asset was
changed during this check.

## Follow-up viewport check

The same open editor was then framed on a visible actor labelled
`TideglassPool`. The view shows a broad, flat white surface at the pool edge
and several separate white spherical forms against the exposed soil and low
groundcover. The Outliner search returned six actors with the
`TideglassPool` label. This appearance is consistent with unresolved/default
materials or blockout geometry, but the actor meshes and material assignments
were not inspected, so their identity and cause are not yet confirmed. The
actor selection and viewport framing were transient editor UI state only; no
map was saved. The current viewport does not provide a trustworthy beauty
capture.

In a subsequent read-only Details-panel check, the selected `TideglassPool`
entry appeared as an instanced actor with a `StaticMeshComponent`. It is not
itself a native WaterBody actor. The selected component's mesh and material
names remained unreadable in the narrow Details panel; this does not identify
the visible white material or prove that all six matching actors are alike.

## Next validation

Inspect the Tideglass-labelled actors' actual mesh and material assignments
in the matching project, including whether any native WaterBody supplies the
actual water surface, then decide whether the white forms are intentional
water/props or unfinished presentation. After that, use a
deliberate player-height route from Tideglass toward woodland with a wide
establishing view and a close groundcover view. Inspect broad soil gaps and
mature canopy silhouettes without inferring Island-wide coverage from one
camera. Compare frame time and whole-device memory before raising instance
counts. The open D: editor project is separate from this worktree, so
worktree-side changes must be deliberately integrated and rebuilt before they
can be evaluated in that editor.
