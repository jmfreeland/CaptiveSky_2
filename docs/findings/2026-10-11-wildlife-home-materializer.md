# Rat wildlife-home materializer — 2026-10-11

## Scope

`FIslandWildlifeHomeRecord` already held renderer-independent home facts, but
loading or storing one did not create any visible shelter. `AIslandWildlifeHome`
now materializes an explicitly saved `Species=Rat` record as the imported
`/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01` mesh. The actor is
transient: the JSON record remains the source of truth, and load/store refreshes
the presentation while record removal destroys it. Reloading the same record
does not create a duplicate actor.

The record's entrance anchor is aligned to the hollow's open front. The asset
origin is ground-centred; its open face is approximately 117 cm along local
negative X from that origin, so the mesh component is offset by 117 cm while
the actor root remains at the saved entrance point and rotation. The geometry
and offset come from the existing `rathollow.py` source and 256 cm asset
manifest.

This is strictly presentation. The mesh has no collision, overlap, navigation
influence, or ticking. `StoreWildlifeHome` does not validate terrain, canopy,
concealment, route, or permission to occupy a site. Unsupported species keep
their canonical record but do not receive this Rat-specific visual. No live
Island home record was added, the Island map was not changed, and Fenrus was
not spawned or assigned a home.

## Verification

The UE 5.8.3 `CaptiveSky_2Editor` target built successfully. The focused
`CaptiveSky2.Agent.WildlifeHomeRecord` automation passed under NullRHI with
thinking disabled, a 60-second ceiling, and zero model requests. Its scratch
world-state file verifies legacy-state compatibility, record persistence,
mesh assignment, entrance anchoring, non-collision/non-navigation, reload
deduplication, and removal of both the record and presentation actor. Log:
[`Codex_WildlifeHomeMaterializer_20261011_2.log`](../../Saved/Logs/Codex_WildlifeHomeMaterializer_20261011_2.log).

## Remaining gate

The materialized asset has not had a real-RHI visual review in a woodland
composition. The editor/MCP is currently unavailable. Before creating a real
`Agent_Rat_01` home record, choose a candidate only after the bounded nav and
canopy audit plus human- and rat-height visual concealment review; keep the
existing empty `places.json` unchanged until that gate passes. Later work can
add resident entry/return and cache materialization without changing this
record/presentation boundary.
