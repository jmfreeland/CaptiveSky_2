# Live checkout parity and recurring Zen startup delay — 2026-11-01

## Finding

The Unreal project currently opened from `D:/Projects - Athena/Unreal/CaptiveSky_2`
is not simply missing one recent fix. A read-only comparison against this
worktree found 378 different files and 30 files missing from D:, out of 516
tracked runtime, config, script, and Unreal project/plugin descriptor files
(108 identical). Differences include the root `.uproject`, all four project
config files, and many asset-pipeline scripts. Treat the D: project and this
source worktree as separate code states; do not assume a committed worktree
change is present in the editor.

The differences are distributed across the checkout, not just pipeline noise:

| Root | Identical | Different | Missing from D: |
| --- | ---: | ---: | ---: |
| project/plugin descriptors | 0 | 1 | 0 |
| `Config/` | 2 | 4 | 0 |
| `Source/` | 97 | 202 | 4 |
| `Scripts/` | 9 | 171 | 26 |

In particular, 202 source files differ. This is too broad for a safe blind
copy; first identify live-only edits in D: and choose an intentional merge or
build/sync strategy. The tracked `.uproject` is included; Content assets are
not part of this source/config/script/descriptor count.

Use the read-only audit before diagnosing a live-editor/source discrepancy:

```powershell
python Scripts/compare_project_source_parity.py "D:\Projects - Athena\Unreal\CaptiveSky_2"
```

Pass project-relative paths to narrow the comparison. The audit hashes files;
it never copies, edits, or saves either checkout. Exit status is 0 when all
compared files match, 1 when differences/missing files are found, and 2 when
the audit cannot run.

## Zen startup evidence

The D: project's `Config/DefaultEngine.ini` lacks the
`[Zen.AutoLaunch] LimitProcessLifetime=False` setting present in this worktree.
The live log `Saved/Logs/CaptiveSky_2-backup-2026.11.01-07.54.36.log` records
Zen waiting for another process's startup/recovery, timing out after 32.507
seconds, then connecting to ZenLocal with status OK. The following
`CaptiveSky_2_2.log` run found no process using the data directory, launched
Zen, and became healthy after 52.602 seconds. This supports recurring recovery
waits and cold starts; it does not establish why the other startup is delayed.

The keep-alive setting is a plausible way to reduce cold starts after the
project config is deliberately synchronized, but it will not necessarily fix
the 32-second wait on another process. No D: files or Zen data were changed in
this audit. Do not delete Zen cache/data or kill processes as a diagnostic
shortcut; capture the relevant Unreal/Zen logs and identify the owner first.

## Verification

- Focused comparison reported `DIFFERENT` for `Config/DefaultEngine.ini`,
  `Source/CaptiveSky_2/CaptiveSky_2Character.cpp`, and
  `Source/CaptiveSky_2/Agent/IslandOceanSubsystem.cpp`. The root
  `CaptiveSky_2.uproject` descriptor is also different.
- Blender 5.2.2 ran the complete `Scripts/tests` suite: 48 tests passed.
- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded: 192 UBT actions,
  including the full module relink. UBT emitted a deprecation warning that
  `MetaHumanGenerator` and the project depend on `StructUtils`, deprecated
  since UE 5.5; it did not fail the build. This compiled the worktree scratch
  target and did not update the open D: editor's loaded module.
- Full read-only audit: 516 files compared; 108 identical, 378 different,
  30 live-missing, 0 source-missing.
- No build was run in the open D: editor project and no rendered gameplay was
  started; this finding makes no runtime claim beyond what the captured
  startup logs establish.

## Vegetation source-parity recheck (2026-11-01)

The current worktree's `IslandWeather.cpp` resolves and scatters five broadleaf
canopy species—Norway maple, European aspen, European beech, black alder, and
Common Hazel—inside seeded, spatially clustered groves. The mature-tree budget
is fixed, and the separate birch/hazel coppice layer is non-colliding. A
read-only SHA-256 comparison against the on-disk D: project's
`Source/CaptiveSky_2/Agent/IslandWeather.cpp` confirms that file is different;
the D: copy is dated 2026-10-29, while the current worktree includes the later
species work. This is consistent with the reported single-species view, but
does not prove the runtime cause or the exact live editor checkout. Do not
copy the file wholesale: the existing 378-file checkout divergence makes that
unsafe without a deliberate source merge.

The current GPU sample was 1,224 MiB free (10,774 MiB used of 12,282 MiB),
below the established 4 GiB reserve. No PIE, second renderer, source sync, or
map edit was attempted. The worktree diversity logic is already present. Once
GPU headroom permits, first inspect live per-species scatter counts and
visibility, then merge any genuinely missing diversity behavior and review
the rendered composition/performance.

**Follow-up detail:** direct read-only inspection of D:'s `IslandWeather.cpp`
shows references to four broadleaf species (maple, aspen, beech, and black
alder), with no Common Hazel reference. The latest D: editor log also contains
a read-only Megaplant material audit reporting nine tree meshes and no audit
errors. It enumerates A/B variants of Norway maple, European aspen, European
beech, and black alder, plus Common Hazel A. The same report shows 24 unique
dependencies under the PVE sample-material mount. This confirms those species
assets are present in the live project's content registry, but not that the
Island instanced them. The PVE material references are a separate packaging
dependency to validate for a hosted build; they are distinct from the retired
PlantFactory mount. The audit confirms asset/material resolution only; it does
not prove the live Island scatter initialized or those species rendered. A
bounded runtime check should record per-species instance counts and visible
components before source syncing more diversity code.

## Next step

**Current desktop recheck:** a read-only app snapshot shows both `CaptiveSky_2 - Unreal Editor` and a separate `Tree_Common_Hazel_01_A` asset-editor window; the mesh preview is focused. This confirms the Hazel asset is open for inspection, not that it has been placed or rendered on the Island. The latest D: log has no `Island tree mix` runtime count entry. The latest GPU sample is 10,774/12,282 MiB used (1,224 MiB free), 43% utilization, 76°C, below the 4 GiB reserve. No editor input, PIE, additional renderer, source sync, or map edit was performed.

Once GPU headroom permits, inspect live per-species instance counts and
component visibility, then merge only any missing diversity behavior. Separately,
agree on a controlled source-sync point for D: (including preservation of any
live-only edits) before syncing the Zen config and comparing a single-editor
startup. Keep water traversal and real-RHI performance as separate runtime
gates.
