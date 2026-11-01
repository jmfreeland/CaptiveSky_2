# Live checkout parity and recurring Zen startup delay — 2026-11-01

## Finding

The Unreal project currently opened from `D:/Projects - Athena/Unreal/CaptiveSky_2`
is not simply missing one recent fix. A read-only comparison against this
worktree found 377 different files and 28 files missing from D:, out of 513
tracked runtime, config, and script files (108 identical). Differences include
all four project config files and many asset-pipeline scripts. Treat the D:
project and this source worktree as separate code states; do not assume a
committed worktree change is present in the editor.

The differences are distributed across the checkout, not just pipeline noise:

| Root | Identical | Different | Missing from D: |
| --- | ---: | ---: | ---: |
| `Config/` | 2 | 4 | 0 |
| `Source/` | 97 | 202 | 4 |
| `Scripts/` | 9 | 171 | 26 |

In particular, 202 source files differ. This is too broad for a safe blind
copy; first identify live-only edits in D: and choose an intentional merge or
build/sync strategy. The `.uproject` and Content assets were not part of this
source/config/script count.

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
  `Source/CaptiveSky_2/Agent/IslandOceanSubsystem.cpp`.
- Blender 5.2.2 ran the complete `Scripts/tests` suite: 48 tests passed.
- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded: 192 UBT actions,
  including the full module relink. UBT emitted a deprecation warning that
  `MetaHumanGenerator` and the project depend on `StructUtils`, deprecated
  since UE 5.5; it did not fail the build. This compiled the worktree scratch
  target and did not update the open D: editor's loaded module.
- Full read-only audit: 513 files compared; 108 identical, 377 different,
  28 live-missing, 0 source-missing.
- No build was run in the open D: editor project and no rendered gameplay was
  started; this finding makes no runtime claim beyond what the captured
  startup logs establish.

## Next step

Agree on a controlled source-sync point for D: (including preservation of any
live-only edits), then rerun the parity audit. After syncing the Zen config,
observe a normal editor close/reopen and compare Zen startup timing. Keep the
water traversal and real-RHI performance checks as separate runtime gates.
