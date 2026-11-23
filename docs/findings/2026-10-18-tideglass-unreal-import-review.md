# Tideglass shoreline plants: isolated Unreal import review (2026-10-18)

## Scope

Imported the local rock-samphire and sea-thrift FBXs into the fresh, isolated
destination `/Game/Generated/ComfyBlender/Codex_TideglassPairReview_20261018/`
using the project's ComfyBlender importer. The destination is shared by these
two meshes and their seven plain-color materials; neither asset imports
textures. No actor was added to the Island and the map was not saved.

## Import evidence

| Candidate | Unreal bounds | Triangles | Materials | Collision |
|---|---:|---:|---:|---:|
| Rock samphire | 27.4 × 46.8 × 40.3 cm | 4,410 | 3 | none |
| Sea thrift | 83.3 × 69.5 × 42.8 cm | 3,846 | 4 | none |

Both importer reports in the local ComfyBlender source folders contain
`validation_errors: []`; UE's content validation completed for both meshes.
The editor log records both static-mesh builds and has no smoothing-group,
tangent, bitangent, or static-mesh build warnings for these imports. The
initially inherited source-root setting pointed at a previous hazel review and
caused two fail-closed `FileNotFoundError`s before any asset import. Setting
the source root explicitly to `Saved/CaptiveSky/ComfyBlender/` resolved it;
both final reports point to the intended source folders and isolated
destination.

## Visual review

The live UE 5.8 Static Mesh Editor previews show the actual imported materials
and geometry. Rock samphire reads as a muted blue-green branching succulent
with small pale-yellow flower heads; its upright scapes are unusually straight,
but its low branching body is distinct from grasses and flowering mats. Sea
thrift has a light-green radial basal tuft, several delicate upright scapes,
and legible pink florets. The latter is a stronger color accent; its basal
leaves still read somewhat like long blades at close range.

This passes the import/mesh-preview gate only. The preview uses Unreal's generic
static-mesh studio scene, not the Island landscape, shoreline, or gameplay
camera. Habitat composition, ground contact, real Island lighting, shadows,
frame-time impact, and play-distance readability have not been validated.
Keep both assets isolated pending a sparse, reversible Tideglass context test;
do not save placements to the Island until that review clears the existing
30-FPS p95 and bounded zero-model-request play safeguards.

## Tideglass site check (2026-10-18)

The live Island contains a small static-mesh basin (`TideglassPool`, centered at
(-99500, 100300, 2880), 160 cm half-bounds) and the Island water zone is at
Z=940. Visibility traces around the east/southeast/south ground found only
Landscape hits, with sampled elevations from Z=2858 to 2952. Candidate spots
near the basin are therefore dry land, not the ocean surface; some of the
northeast/west approaches slope noticeably and should be avoided for a first
composition pass. This is site evidence, not a visual habitat approval.

## Temporary in-world review

Placed three collision-disabled static-mesh actors in the open editor only:
two sea-thrift clumps at the east and south sides of the basin and one rock
samphire on the southeast shoulder (1.2× scale). Their origins were set to
Visibility-traced Landscape elevations, and the saved editor view confirms that
they sit on the terrain without obvious sinking or floating. In daylight, the
pink sea-thrift florets give a legible accent; the samphire's pale, branching
silhouette is distinct but quieter and small at this framing. In this
editor-world view, the basin's white saved footprint and nearby blockout props
dominate the composition. The white top is an editor placeholder, not the
runtime pool appearance: `UIslandTideglassSubsystem` hides it and creates the
transient teal pool surface at Game/PIE start when the generated material is
available. The provider-free Game capture at
`Saved/Playtests/Codex_WorldLifeReview_20261010/Screenshots_TideglassOverlook/007_Habitat__Overlook.png`
shows that rendered water. Therefore the “art the basin” concern applies to the
editor/blockout presentation; runtime landmark polish still needs a current
gameplay camera and performance review.

The view capture is
`Saved/Playtests/Codex_TideglassPairContext_20261018/TideglassPairPreview.png`.
All three temporary actors were destroyed and `/Game/Maps/Island` reloaded;
the editor returned to 180 actors and “All Saved.” The Island map was not
saved with the test placements. Unreal had earlier created
`Saved/Autosaves/Game/Maps/Island_Auto1.umap` while an empty temporary actor
from the first, failed preview attempt was present. That recovery artifact was
left untouched; avoid restoring it without removing the blank actor first.

This passes a static editor-context and ground-contact review only. Gameplay
camera readability, real play, and the existing 30-FPS p95 / bounded
zero-model-request safeguards are not measured. Keep the assets isolated until
those checks pass; do not commit permanent Island placements yet.
