# Legacy PCG forest status

## Island map: disabled submerged forest

The saved `/Game/Maps/Island` contains the old `PCG_ForestVolume_Island` using
`/Game/PCG/PCG_World_Forest`. A read-only reload after the cleanup confirmed
its PCG component is `activated=False`; its five generated ISM components are
hidden and have collision disabled. The generated data is retained, not
deleted. The audit found 3,114 tree instances crossing the ocean plane and 135
fully submerged because the sampled landscape in that region lies below the
water surface. This is the confirmed source of the floating/submerged
appearance reported earlier.

Evidence: `Saved/Logs/Codex_PCGForestCollisionVerifyFinal_20261011.log` and
`docs/findings/2026-10-10-rat-hollow-site-audit.md`.

## Separate PCG test map: active volume, no instanced output on load

A read-only UE 5.8.3 reload of `/Game/Maps/PCG_World` found one
`PCG_ForestVolume` at `(0, 0, 500)` with extent `(12,000, 12,000, 1,500)` cm.
Its assigned graph is the same `PCG_World_Forest`; its component reports
`activated=True` and `generated=True`. However, a scan of all 1,054 loaded
actors found **zero `InstancedStaticMeshComponent` outputs** at that point.
This says the separate map can run the legacy graph but did not have generated
foliage instances attached on the inspected load. The `generated` flag alone
does not prove visible output.

The inspection was read-only and did not save either map or graph. The separate
map and graph remain preserved. The live Unreal MCP connection was unavailable,
so the current interactive editor viewport was not verified. If the forest is
still visible in an open session, inspect that session after a clean Island
reload; do not delete the graph or PCG map based on the old floating appearance.

Evidence: `Saved/Logs/Codex_PCGWorldReadOnlyFinal_20261011.log` and the
read-only inventory script at
`Saved/Playtests/Codex_PCGWorldAudit_20261011/Inspect-PCGWorld.py`.
