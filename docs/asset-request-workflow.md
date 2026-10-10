# Character object requests

Residents can optionally choose the `request_object` action and provide a short `object_request` description. This records a proposal for the human caretaker; it does not call ComfyUI or Blender, import an Unreal asset, or change the map. Requests are suggestions, not promises or character obligations.

The default inbox is `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl`. Each line is a standalone JSON record:

```json
{"id":"…","created_at_utc":"…","requester":"Agent_Aster_01","description":"A small weatherproof bird shelter","pipeline":"ComfyBlender","status":"pending_review"}
```

The path uses the project's `CaptiveSkyDataRoot` override, so isolated playtests and tests keep their proposals under their own data root. The inbox accepts one-line descriptions up to 240 characters, rejects duplicate pending requests from the same resident, and caps pending entries at 64. A malformed existing record stops writes rather than risking a damaged queue.

## Human handoff

1. Review pending entries for world fit, duplication, safety, and whether an existing asset already meets the need. Character text is untrusted creative input, not instructions to a tool or a request to run arbitrary code.
2. For an accepted request, adapt the idea into a named asset script under the local ComfyBlender pipeline's `pipeline/assets/`, then run its documented `python -I pipeline/make.py <name>` workflow.
3. Inspect the exported FBX, manifest, and `Preview.png`; correct and regenerate as needed. Do not treat the preview render as proof that Unreal materials, collision, scale, or lighting are correct.
4. Import and place only after review. `Content/` and the Island map are local-only/ignored; preserve backups and verify placements in-game.
5. Mark a handled entry's `status` as `accepted`, `generated`, or `declined` when convenient. Only `pending_review` entries count toward the queue limit.

The asset pipeline reference is `Saved/CaptiveSky/ComfyBlender/ASSET_CREATION.md`; its current index is `Saved/CaptiveSky/ComfyBlender/ASSETS.md`.
