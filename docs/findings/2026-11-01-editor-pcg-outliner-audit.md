# Live PCG Outliner audit — 2026-11-01

The connected UE 5.8 editor is open on the `Island` map. A read-only Outliner
filter for `PCG` showed one actor, `PCGWorldActor`; filtering for `Forest`
showed no separately named forest actor. The Outliner filter was cleared after
the check. No actor was selected, moved, deleted, or edited, and the map was
not saved or played.

This is not proof that the old floating forest is absent. PCG output or baked
instances could live in a differently named actor, a landscape/component, or a
generated partition. `PCGWorldActor` is the only result seen here; its presence
alone does not establish that it owns the reported forest. The viewport was
still aimed across open water, so it did not visually validate the forest
placement.

Next, inspect PCG component/graph owners and landscape foliage while the Island
is framed in the viewport. Preserve the global `PCGWorldActor`; only remove a
specific obsolete generated output after its ownership and floating placement
are confirmed. Rendered play remains gated on at least 4 GiB of free VRAM.
