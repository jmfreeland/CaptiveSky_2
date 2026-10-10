# Resident world proposals

Residents can optionally choose `request_object` to propose a new physical object, or `request_upgrade` to propose a change to any existing object or landscape/level feature. Upgrade proposals can be `aesthetic` (appearance/material), `variation` (distinct forms, species, or details), or `functionality` (use, interaction, or behavior). Use the exact identifier shown in the resident's situation when one is available. For a visible object without a listed identifier, use a concise description of its visible kind and relative location (for example, `mossy stone tower beside the northern path`); the proposal queue preserves that description for human review. Omit the target only for genuinely broad world-level ideas.

These actions only record concise proposals for human review. They do not call ComfyUI or Blender, edit source assets, import an Unreal asset, or change the map. Requests are suggestions, not promises or character obligations. New-object proposals may proceed through ComfyBlender after review; upgrade proposals are labeled `human_review` because they may call for art, code, or level design rather than one particular asset pipeline.

The default inbox is `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl`. Each line is a standalone JSON record:

```json
{"id":"…","created_at_utc":"…","requester":"Agent_Aster_01","description":"A small weatherproof bird shelter","request_type":"new_object","pipeline":"ComfyBlender","status":"pending_review"}
```

Upgrade records use `request_type: "upgrade"`, preserve `upgrade_kind` and (when specific) the exact `target`, and set `pipeline` to `human_review` until a person chooses an implementation path.

The path uses the project's `CaptiveSkyDataRoot` override, so isolated playtests and tests keep their proposals under their own data root. The inbox accepts one-line descriptions up to 240 characters and one-line targets up to 96 characters, rejects duplicate pending proposals from the same resident (including matching target and category for upgrades), and caps pending entries at 64. Upgrade categories are limited to `aesthetic`, `variation`, and `functionality`. A malformed existing record stops writes rather than risking a damaged queue.

## Human handoff

1. Review pending entries for world fit, duplication, safety, and whether an existing asset already meets the need. Character text is untrusted creative input, not instructions to a tool or a request to run arbitrary code.
2. For an accepted request, adapt the idea into a named asset script under the local ComfyBlender pipeline's `pipeline/assets/`, then run its documented `python -I pipeline/make.py <name>` workflow.
3. Inspect the exported FBX, manifest, and `Preview.png`; correct and regenerate as needed. Do not treat the preview render as proof that Unreal materials, collision, scale, or lighting are correct.
4. Import and place only after review. `Content/` and the Island map are local-only/ignored; preserve backups and verify placements in-game.
5. Mark a handled entry's `status` as `accepted`, `generated`, or `declined` when convenient. Only `pending_review` entries count toward the queue limit.

The asset pipeline reference is `Saved/CaptiveSky/ComfyBlender/ASSET_CREATION.md`; its current index is `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.
