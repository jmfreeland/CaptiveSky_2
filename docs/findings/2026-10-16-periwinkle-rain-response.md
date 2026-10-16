# Periwinkle response to heavy rain — 2026-10-16

Tideglass periwinkles already respond to the exposed ebb and to close, quiet
observation, but unlike the fireflies they did not read the Island's weather.
They now sample the existing deterministic `AIslandWeather` rain signal at the
same 0.2-second tick interval and draw their eye stalks, aperture, and gentle
body motion in slightly during a heavy shower. The effect is bounded to at most
0.42 rest, so it layers under the stronger ebb/observation tuck and never
removes the snail or changes its home. In dry weather the response is zero.

This is presentation-only ecology: no movement, memory, world-state, or model
request is added. If no weather actor is present, the periwinkle keeps its
previous behavior.

## Validation

- UE 5.8.3 editor target built in the isolated
  `Saved/CompileScratch/Claude_Props` project; UHT passed and the changed
  periwinkle implementation and `IslandEcologyTests.cpp` compiled and linked.
- `CaptiveSky2.Agent.NightEcology` passed under `UnrealEditor-Cmd -NullRHI`
  with exit code 0. Its fixture compares the same periwinkle under a seeded
  heavy shower and dry weather, checks that the stalks tuck slightly and then
  return to full extension, and tests the pure response's bounds, monotonicity,
  and non-finite input handling.
- Log: [`Codex_NightEcology_RainResponse_20261016.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NightEcology_RainResponse_20261016.log).

This does not yet show whether the small visual cue reads clearly in ordinary
play. Inspect it at Tideglass during a naturally occurring shower once the
Island editor/game path is available; keep the standard playtime and model-call
caps in place.

## Bounded real-RHI review — 2026-10-16

The scratch project successfully loaded the actual `/Game/Maps/Island` for two
standalone Game checks without changing or saving the authored map. Both used
an isolated `CaptiveSkyDataRoot`, disabled resident thinking, set the model
request limit to zero, forced a short storm, and exited at their real-time cap:

- A 90-second broad Tideglass pass ended after 90.3 seconds with zero model
  requests. It captured five 1600x900 frames at 8-second intervals. The storm
  atmosphere and pool are visible, but these views are too wide to judge the
  snail response. Log and frames:
  [`game log`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Logs/PeriwinkleRainGame.log),
  [`001`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Screenshots/001_Tideglass.png),
  [`005`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Screenshots/005_Tideglass.png).
- A 45-second close Tideglass pass ran with Python disabled and ended after
  45.2 seconds with zero model requests. Its frames show the shell, but tall
  foreground plants obscure the eye stalks and most of the animal; the
  rain-driven tuck therefore remains **not visually verified**. The same
  capture displays `TEXTURE STREAMING POOL OVER 4.449 MiB BUDGET`, so this
  close frame is also evidence of a remaining texture-budget problem, not a
  candidate hero shot. Log and frame:
  [`close-up log`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Closeup/Logs/PeriwinkleCloseup.log),
  [`close-up frame`](../../Saved/Playtests/Codex_PerwinkleRain_20261016_144027/Closeup/Screenshots/006_Perwinkle__Close.png).

The visible gap is now specific: periwinkles exist and respond in deterministic
automation, but their current ground-traced homes can sit behind much taller
cover, making the small expressive parts unreadable from a close visitor view.
Next, inspect how the close shoreline vegetation and texture pool can be
art-directed to preserve a legible rocky microhabitat before enlarging the
primitive animal or claiming the rain behavior reads in play. No authored map
or Content asset was changed; the custom viewpoint and all runtime data remain
local under `Saved/Playtests/`.

## Ground-cover sightline pass — 2026-10-16

Ground-cover initialization now shares the exact deterministic nine-point
shoreline candidate sequence used by the three periwinkles. Before scattering,
it traces each candidate against the same shoreline collision and records the
first grounded home; only full-height grass whose visual footprint overlaps a
125 cm resident-scale margin is withheld. Short shoreline understorey, the
species budget, the three-resident population, and all authored map/Content
assets remain unchanged. This reserves small readable pockets rather than
clearing broad swathes of the Tideglass meadow.

The scratch `CaptiveSky_2Editor` target rebuilt successfully with the editor
left open. `CaptiveSky2.Agent.NightEcology` passed (exit 0), and its shared
home-selection code was exercised. The enabled `CaptiveSky2.Agent.GroundCover`
fixture logged three grounded homes and seven tall-grass candidates skipped;
its new integration assertion passed. That broader test still exits 1 on four
existing assertions: one variant-visibility check (including two absent
understorey variants) and three resident-sway checks. A control build with the
clearing disabled produced the same four failures, so they are not regressions
from this pass. Logs:
[`NightEcology`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_PeriwinkleClearance_NightEcology_20261016.log),
[`enabled GroundCover`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_PeriwinkleClearance_GrassOnly_20261016.log),
[`no-clearing control`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_PeriwinkleClearance_BaselineCompare_20261016.log).

Visual confirmation is still pending. The last close-up was obscured by tall
foreground plants and over texture budget; rerun a capped, zero-request real-RHI
close-up when the open editor is no longer needed, then tune the clearing only
if the resident still fails to read naturally against the shore.
