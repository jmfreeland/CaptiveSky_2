# Fenrus hollow site audit (2026-10-10)

The user chose a **hidden hollow in fallen wood** as Fenrus' first home. The
mesh is available locally at
`/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01`; the source cap was
removed and reimported into the live UE 5.8 editor. The editor reports 1,836
triangles, 256.4 x 135.7 x 134.1 cm bounds, and four box collision shapes. The
import report has no validation errors. In the mesh editor's Back view, the
entrance is open with a dark interior. This validates the asset import and
visible opening, not ground contact, navigation, or gameplay fit.

## Locations checked

- `Roost_East_FallenWood` at `(-99850, 101980, 2767)` is a real fallen-wood
  prop, but it belongs to the raven's roost cluster. Live viewport captures
  show the small saplings/undergrowth around it do not conceal the area; the
  surrounding meadow is open. Reject it as Fenrus' home: it would reuse the
  raven's site and does not meet the hidden-home brief.
- The woodland-fox candidate `(-100915, 97968, 2869)` was also viewed at ground
  level. The nearby slope is open grass facing the sea and is not a hidden
  hollow site.

Both checks used `EditorToolset.EditorAppToolset.CaptureViewport` on the live
`/Game/Maps/Island` editor world. `SceneTools.trace_world` returned ground or
prop hits around the roost, but no candidate has had navigation clearance
verified. The Island map is unchanged and no coordinate was written to
`Agents/Agent_Rat_01/places.json` (still empty).

## Procedural vegetation

`PluginToolset.PluginToolset.IsEnabled` confirms
`ProceduralVegetationEditor` is enabled in the running editor. The separate
`PlantFactoryPlugin` is enabled in the project descriptor but is not discovered
by this live session; it needs a project restart before its content can be
verified. The earlier [PVE availability probe](2026-10-10-pve-availability.md)
established that PVE and the PlantFactory content plugin load in an isolated
UE 5.8.3 scratch project, but did not validate interactive authoring or output
quality.

## Next step

Find or build a separate, genuinely sheltered fallen-wood pocket, then verify
the entrance orientation, terrain contact, and rat-sized navigation clearance
before recording a location or spawning Fenrus. Do not reuse the Raven's roost
just because its fallen-wood actor is nearby. A small PVE/Megaplant preview is
appropriate only after the editor session has the PlantFactory mount and the
selected pocket can be checked at gameplay scale.
