# Asset pipeline build serialization (2026-10-19)

`Scripts/AssetPipeline/make.py` now holds one Windows exclusive-open lock for the
entire requested build batch. The lock is placed beside the shared export root,
so manual `make.py` runs and accepted resident-request builds use the same gate
when they target the same exports. The OS releases the lock when the process
exits, including an abnormal exit; the small `.asset-pipeline.lock` file is only
a stable lock target and does not itself indicate that a build is active.

If another build owns the export root, the orchestrator waits up to five seconds,
then exits with a clear error before running Blender or rewriting an asset log.
This prevents overlapping Blender builds from competing for the machine's GPU
and avoids concurrent writes to shared export/intermediate files. It does not
serialize callers that intentionally configure different export roots, and it
does not protect scripts that bypass `make.py` and invoke Blender directly.

The focused Windows test verifies mutual exclusion, release after the critical
section, and that `make.py` keeps the lock held across every asset in a batch.
It mocks the asset build itself, so it does not launch Blender or generate an
asset. Unreal import, visual review, and gameplay impact remain separate gates.

Verification: the focused build-lock tests passed (2/2), then the full local
AssetPipeline helper suite passed (94/94) using the workspace-bundled Python
with temporary files redirected under `Saved/Tests/Temp`.
