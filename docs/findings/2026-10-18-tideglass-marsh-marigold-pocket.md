# Tideglass marsh-marigold pockets (2026-10-18)

> **Superseded later on 2026-10-18:** this note records the earlier marigold-only
> 4-of-8 pocket trial. The current source uses three marsh-marigold pockets,
> two yellow-flag iris pockets, and three pockets from the established flower
> mix, still capped at 24 Tideglass flowers. The later source and test record is
> [Tideglass yellow-flag iris runtime trial](2026-10-18-tideglass-yellow-flag-iris-runtime-trial.md).
> This changes runtime species selection only; no Island map placements were
> saved. Gameplay readability and the 30-FPS p95 gate remain open.

## Earlier marigold-only change (superseded)

The imported `/Game/Generated/ComfyBlender/TideglassMarshMarigold_Review_20261018/SM_TideglassMarshMarigold_01` mesh now has a dedicated, collisionless HISM slot in `AIslandWeather`. Four of the eight deterministic Tideglass pollinator sites alternate to this freshwater-margin species; each selected site keeps its existing three-member flower pocket. The other sites retain the previous locally varied flower choice. The pattern is seed-stable and flips with world-seed parity.

This is a substitution, not a density increase: the eight pollinator sites still permit up to 24 instances total, and the general flower-site budget is unchanged. The marigold is excluded from the dry-meadow selector; all nine existing meadow/woodland flower forms retain their original distribution. If the local imported marigold asset is unavailable, Tideglass sites fall back to the existing flower mix. The imported candidate is approximately 1,968 triangles with four plain material slots and no textures; this adds one HISM component but no texture memory or collision.

## Earlier validation

- UE 5.8.3 scratch target build succeeded. The source junction points at the current checkout; the scratch project has its own `Binaries` and `Intermediate`, so the user's main project DLL and saved map were not rebuilt or saved.
- `CaptiveSky2.Agent.MeadowFlowerSpecies` passed, verifying the imported asset resolves, the dedicated HISM references it and remains non-colliding/outside navigation, the site split is deterministic and balanced, and the general picker excludes marigold.
- `CaptiveSky2.Agent.GroundCover` passed under NullRHI after its rendering-component and nine-form distribution assertions were updated for the new niche-only slot. The run used `-CaptiveSkyDisableAgentThinking -CaptiveSkyMaxModelRequests=0` and did not save the Island.
- Logs: `Saved/Logs/Codex_TideglassMarigold_20261018.log` and `Saved/Logs/Codex_TideglassMarigold_GroundCover_Final_20261018.log`.

## Limits and next review

The GroundCover test fixture has no real Island landscape, so it verifies component setup and deterministic selection—not that the saved Island actually gets four marigold pockets. The main editor session was reported responsive by the user, but it was not exposed in this session's desktop/process inventory. No placement was saved and no Island gameplay frame, waterline composition, play-distance readability, texture-streaming budget, frame-time, or performance target was measured.

Next, in the live Island, capture the Tideglass area at the intended daylight and water level; confirm the selected pockets sit on appropriate damp ground and that the blooms read at player distance. Then run the existing bounded, zero-model-request fixed-view performance check before considering any permanent art adjustment. The imported asset is local ignored Content, so this source reference assumes the project-local review asset remains installed; the runtime falls back if it is absent.
