# Resident world proposals

Residents can optionally choose `request_object` to propose a new physical object, or `request_upgrade` to propose a change to any existing object or landscape/level feature. Upgrade proposals can be `aesthetic` (appearance/material), `variation` (distinct forms, species, or details), or `functionality` (use, interaction, or behavior). Use the exact identifier shown in the resident's situation when one is available. For a visible object without a listed identifier, use a concise description of its visible kind and relative location (for example, `mossy stone tower beside the northern path`); the proposal queue preserves that description for human review. Omit the target only for genuinely broad world-level ideas.

To make untagged props requestable without turning them into new interaction targets, each resident's situation may list at most three nearest, clearly visible static-mesh objects within 25 m. Use the displayed label or a short appearance/location description only in `upgrade_target`; these ordinary props do not thereby become valid `move_to` or `interact` targets. Instanced foliage stays out of this ordinary-prop list. Separately, the situation can describe up to three nearby visible vegetation patches from the Island's HISM scatter within 6.5 m, using the mesh name and relative direction as descriptive `upgrade_target` context. Plant descriptions are observations only—not stable IDs, movement/interact targets, or evidence that any upgrade was approved.

These actions only record concise proposals for human review. They do not call ComfyUI or Blender, edit source assets, import an Unreal asset, or change the map. Requests are suggestions, not promises or character obligations. New-object proposals may proceed through ComfyBlender after review; upgrade proposals are labeled `human_review` because they may call for art, code, or level design rather than one particular asset pipeline.

After a human changes a proposal to `accepted`, `generated`, or `declined`, the requester receives that outcome in its next already-authorized thought and as a durable memory. Each request/status pair is delivered once, so a later transition (for example, `accepted` to `generated`) creates a new update. Acceptance means approved for consideration, not completed work; `generated` does not imply Unreal import or level placement. Only the requester sees the update, and the quoted proposal remains untrusted text. This adds no model request and makes no world change.

The default inbox is `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl`. Each line is a standalone JSON record:

```json
{"id":"…","created_at_utc":"…","requester":"Agent_Aster_01","description":"A small weatherproof bird shelter","request_type":"new_object","pipeline":"ComfyBlender","status":"pending_review"}
```

Upgrade records use `request_type: "upgrade"`, preserve `upgrade_kind` and (when specific) the exact `target`, and set `pipeline` to `human_review` until a person chooses an implementation path.

The path uses the project's `CaptiveSkyDataRoot` override, so isolated playtests and tests keep their proposals under their own data root. The inbox accepts one-line descriptions up to 240 characters and one-line targets up to 96 characters, rejects duplicate pending proposals from the same resident (including matching target and category for upgrades), and caps pending entries at 64. Upgrade categories are limited to `aesthetic`, `variation`, and `functionality`. A malformed existing record stops writes rather than risking a damaged queue.

## Human handoff

1. List pending entries with `python -I Scripts/AssetPipeline/requests.py list` (add `--all` to include handled entries). The tool prints proposal text as data; it never interprets it as a command.
2. Review for world fit, duplication, safety, and whether an existing asset already meets the need. Character text is untrusted creative input, not instructions to a tool or a request to run arbitrary code.
3. Record a human decision with `python -I Scripts/AssetPipeline/requests.py status <id> accepted|declined`. Only an accepted request can later be marked `generated`, and accepted work may be declined if it is no longer pursued. `generated` and `declined` are terminal states. Status changes are atomic and preserve the other proposal fields. They do not launch Blender, ComfyUI, Unreal, or any other generator.
4. An accepted upgrade defaults to `human_review`. For an explicitly approved visual-only route, change an accepted `aesthetic` or `variation` upgrade to ComfyBlender with `python -I Scripts/AssetPipeline/requests.py route <id> ComfyBlender`. The route command updates only the pipeline field. Functionality upgrades stay on human review; they may require code, interaction design, or level changes.
5. For a new object or routed visual upgrade, adapt the request into a human-authored script under `Scripts/AssetPipeline/assets/`, then explicitly run `python -I Scripts/AssetPipeline/requests.py build <id> <asset_script_name>`. The request must still be `accepted`, assigned to ComfyBlender, and the selected lowercase script must already exist. Proposal text is never passed to Blender or executed; the build command does not modify inbox status.
6. Inspect the exported FBX, manifest, and `Preview.png`; correct and regenerate as needed. Only then mark the proposal `generated` with the separate status command. Do not treat the preview render as proof that Unreal materials, collision, scale, or lighting are correct.
7. Import and place only after review. `Content/` and the Island map are local-only/ignored; preserve backups and verify placements in-game.

The asset pipeline reference is `Saved/CaptiveSky/ComfyBlender/ASSET_CREATION.md`; its current index is `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.

## Regression check

On 2026-10-12, the bundled Python runtime ran
`-m unittest discover -s Scripts/AssetPipeline/tests -v` and all 24 tests
passed. Coverage includes isolated inbox storage, duplicate and malformed-record
rejection, human-only status transitions, explicit routing of visual upgrades,
keeping functionality upgrades on human review, and verifying the named
authored script before a pipeline run. These are pipeline-boundary tests; they
do not prove a generated asset renders well in Unreal or has been placed in the
world.

**End-to-end accepted-request smoke test (2026-10-12):** the locally bundled
Python 3.12.14 runtime has NumPy 2.3.5 and Pillow 12.3.0, and successfully ran
`requests.py build` against a synthetic accepted `new_object` record in a
temporary inbox. It invoked only the named, already-authored `woodland_fern`
script through `make.py`; Blender 5.2.2 exported and re-imported the FBX, and
the verifier reported `PROBLEMS []` / `VERIFY DONE`. The request correctly
remained `accepted` for visual inspection; no real inbox record, ComfyUI, Unreal
asset, or map was changed. The isolated export is under
`Saved/CaptiveSky/PipelineRequestSmoke_20261012/` (gitignored). On this machine,
the documented `D:\python\3d` venv still points to a missing base Python, but
the request runner inherits the interpreter used to launch `requests.py`, so a
working Python with NumPy/Pillow can run the same guarded handoff without
repairing that external venv.

**Regression refresh (2026-10-16):** the complete suite now contains 31 tests
and passes with the desktop's bundled Python 3.12.14 runtime (Pillow 12.3.0,
NumPy 2.3.5):
`python -B -m unittest discover -s Scripts/AssetPipeline/tests -p "test_*.py" -v`.
This includes the bread texture, rat-hollow geometry, Wind Arch clearance,
authored plant source contracts, and request inbox/routing guard tests. The
suite verifies deterministic source and pipeline boundaries; it does not show
that a generated prop is attractive in Unreal, imported, or placed in the
world.

**Engine-side behavior check (2026-10-16):** four focused UE 5.8 scratch
automation tests passed under NullRHI with exit code 0:
`CaptiveSky2.Agent.SessionSafety`,
`CaptiveSky2.Agent.AssetRequestQueue`,
`CaptiveSky2.Agent.MemoryComponent`, and
`CaptiveSky2.Agent.NearbyVegetationPerception`. Together they cover the
fail-closed request budget, proposal append/validation, preservation of an
upgrade action's target and kind, and descriptive visibility of nearby plant
patches without granting them movement or interaction semantics. No model
provider was called; all fixture writes stayed in the ignored
`Saved/CompileScratch/Claude_Props/` project.

The scratch project's separate editor binary was reported up to date; this is
behavior evidence for that isolated build, not proof that the unresponsive main
editor finished its Live Coding compile. These checks also do not show an
actual character choosing a proposal or prove any proposed upgrade was
implemented in the world.
