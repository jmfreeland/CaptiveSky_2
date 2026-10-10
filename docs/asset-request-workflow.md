# Resident world proposals

Residents can optionally choose `request_object` to propose a new physical object, or `request_upgrade` to propose a change to any existing object or landscape/level feature. Upgrade proposals can be `aesthetic` (appearance/material), `variation` (distinct forms, species, or details), or `functionality` (use, interaction, or behavior). Use the exact identifier shown in the resident's situation when one is available. For a visible object without a listed identifier, use a concise description of its visible kind and relative location (for example, `mossy stone tower beside the northern path`); the proposal queue preserves that description for human review. Omit the target only for genuinely broad world-level ideas.

To make untagged props requestable without turning them into new interaction targets, each resident's situation may list at most three nearest, clearly visible static-mesh objects within 25 m. Use the displayed label or a short appearance/location description only in `upgrade_target`; these ordinary props do not thereby become valid `move_to` or `interact` targets. Instanced foliage is excluded from this prop list.

These actions only record concise proposals for human review. They do not call ComfyUI or Blender, edit source assets, import an Unreal asset, or change the map. Requests are suggestions, not promises or character obligations. New-object proposals may proceed through ComfyBlender after review; upgrade proposals are labeled `human_review` because they may call for art, code, or level design rather than one particular asset pipeline.

The default inbox is `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl`. Each line is a standalone JSON record:

```json
{"id":"…","created_at_utc":"…","requester":"Agent_Aster_01","description":"A small weatherproof bird shelter","request_type":"new_object","pipeline":"ComfyBlender","status":"pending_review"}
```

Upgrade records use `request_type: "upgrade"`, preserve `upgrade_kind` and (when specific) the exact `target`, and set `pipeline` to `human_review` until a person chooses an implementation path.

The path uses the project's `CaptiveSkyDataRoot` override, so isolated playtests and tests keep their proposals under their own data root. The inbox accepts one-line descriptions up to 240 characters and one-line targets up to 96 characters, rejects duplicate pending proposals from the same resident (including matching target and category for upgrades), and caps pending entries at 64. Upgrade categories are limited to `aesthetic`, `variation`, and `functionality`. A malformed existing record stops writes rather than risking a damaged queue.

## Human handoff

1. List pending entries with `python -I Scripts/AssetPipeline/requests.py list` (add `--all` to include handled entries). The tool prints proposal text as data; it never interprets it as a command.
2. Review for world fit, duplication, safety, and whether an existing asset already meets the need. Character text is untrusted creative input, not instructions to a tool or a request to run arbitrary code.
3. Record a human decision with `python -I Scripts/AssetPipeline/requests.py status <id> accepted|declined`; after successful generation, `generated` can mark completion. Status changes are atomic and preserve the other proposal fields. They do not launch Blender, ComfyUI, Unreal, or any other generator.
4. For an accepted new-object request, adapt the idea into a named asset script under the local ComfyBlender pipeline's `pipeline/assets/`, then run its documented `python -I pipeline/make.py <name>` workflow.
5. Inspect the exported FBX, manifest, and `Preview.png`; correct and regenerate as needed. Do not treat the preview render as proof that Unreal materials, collision, scale, or lighting are correct.
6. Import and place only after review. `Content/` and the Island map are local-only/ignored; preserve backups and verify placements in-game.

The asset pipeline reference is `Saved/CaptiveSky/ComfyBlender/ASSET_CREATION.md`; its current index is `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.
