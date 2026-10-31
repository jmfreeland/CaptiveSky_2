# Woodland scatter overlap

The woodland generator placed several layers in distinct annuli around each
grove center: mature trees at 5–16 m, saplings at 17–26 m, shrubs at 7.5–20.5
m, flowering rhododendrons at 21–36 m, and coppice at 23–30 m. Those repeated
radial bands risk reading as nested planted rings instead of an irregular,
mixed woodland.

The candidate footprints now overlap: mature trees 0–18 m, saplings 8–28 m,
shrubs 2.5–24 m, rhododendrons 6–35 m, and coppice 9–32 m from the existing
grove centers. The fixed instance budgets, spacing and landmark clearances,
species selection, collision policy, and cull distances are unchanged. This
changes placement only; it adds no meshes or instances, so it does not raise
the vegetation budget or GPU cost by design.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` target compiled successfully
against the isolated `Saved/CS58` project copy, whose content is a read-only
junction to the main project's assets. The code now includes a focused
`BroadleafDiversity` assertion that all five layer ranges have a shared radial
habitat; it compiled with the change. Two Editor-Cmd automation attempts
remained live for two minutes, emitted only SDK-preflight output, and never
created their project logs; they were stopped, so no automation pass is
claimed. A rendered comparison is also pending: the live editor had about 2.5
GiB of GPU memory free, below the project's 4 GiB preview gate.

Next: once memory permits a guarded run, compare the same woodland view and
record per-species counts, visible clumping, frame-time p95, and VRAM. If the
overlap looks too dense or still too regular, tune the spatial field rather
than adding more vegetation instances.
