# Rat Hollow silhouette pass (2026-10-11)

The in-editor trial view showed the first hollow as a pale, very regular log on
bare ground. The generator now gives the outer shell a more uneven radial
profile, a small drifting centerline, and a broken rear root. Moss clumps are
less evenly spaced and include one low side patch. The entrance, inner tunnel,
simple collision, and Rat capsule-clearance dimensions are intentionally
unchanged.

The new test keeps the shell's longitudinal radius/height variation and checks
the same swept 16 x 24 cm capsule route through the collision. The Blender
5.2.2 build exported a 259 x 140 x 133 cm mesh (about 1,960 triangles), and a
separate headless FBX re-import found one UV channel on the render mesh plus
five box-collision meshes. The candidate is at
`Saved/CaptiveSky/ComfyBlender/RatHollow_ArtPass_20261011/RatHollow/`; it did
not overwrite the already imported trial asset. No preview render or Unreal
visual review was run because the opened editor has unrelated unsaved asset
work. This pass did not edit a map or resident data.

Next: review the regenerated export preview, then compare it in the disposable
Island trial with runtime understory enabled. Keep the main Island and
`Agents/Agent_Rat_01/places.json` unchanged until concealment, ground contact,
and Rat movement/navigation all pass.
