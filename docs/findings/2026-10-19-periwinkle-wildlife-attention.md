# Tidepool periwinkle notices nearby wildlife

## Change

At a spatially staggered 0.4-second check, each periwinkle now chooses the nearest
front-facing Common Frog or Tideglass Dragonfly within 220 cm and the local
vertical attention band. Its eye stalks ease toward that animal, then settle
back naturally when it leaves range. A nearby player keeps priority. This uses
the existing slow tick, adds no per-frame world scan or visibility trace, and
does not move animals, save state, or request a model response.

This extends the existing frog/dragonfly relationship with a small
inter-species cue at the tidepool edge. The bounds and gaze math have focused
automation coverage; they do not prove the behavior is legible at ordinary
play distance. A close Tideglass view in a later bounded, zero-request run is
still needed before tuning frequency or art.

## Verification

The isolated UE 5.8 `CaptiveSky_2Editor` scratch target built successfully in
42.24 seconds (10 actions), including the changed Periwinkle implementation
and automation test unit. The focused
`CaptiveSky2.Agent.TidepoolPeriwinkleWildlifeAttention` automation was not run:
the open editor's command console was not reachable through desktop
interaction, and Unreal MCP at `127.0.0.1:8000` remained unavailable. No
Island map or persistent world-state data was changed. The next verification
is the focused automation plus a close, bounded zero-request Tideglass view.
