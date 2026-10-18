# Wind Arch visitor response — 2026-10-18

The Wind Arch now has a small non-verbal response to the player: a slow visit
within 380 cm and 240 cm of its elevation can release one brief pass of the
existing wind motes. The effect lasts eight seconds, travels along the local
wind direction (or the Arch's forward direction in calm weather), and shares
the existing nearby-effect guard with natural gusts. A visitor must leave a
650 cm ring before another visit can trigger; a separate 36-second cooldown
also limits responses. Fast movement does not trigger the response. This adds
no model request, persistent record, map edit, or new asset.

## Validation

- A UE 5.8.3 `CaptiveSky_2Editor` build succeeded from an isolated project
  snapshot under `Saved/CompileScratch/Codex_WindArchVisitor_20261018`. The
  scratch project has its own `Binaries` and `Intermediate`; its `Source`,
  `Config`, and `Plugins` are copies of the current checkout. The first cold
  compile caught an unqualified private test constant; the corrected test
  translation unit compiled and the four-action incremental build linked the
  module successfully with `-NoHotReloadFromIDE -MaxParallelActions=3`. The
  main editor's loaded module was not relinked.
- The focused automation and visual/PIE behavior remain unverified. The live
  editor window was present but could not be activated, and the desktop capture
  showed Edge instead. The scratch project was intentionally compile-only; it
  does not contain the Island map. No editor process was stopped.

When a responsive editor is available, verify one approach produces one pass,
lingering does not retrigger it, leaving/re-entering after cooldown can produce
another, and a strong-wind pass is not doubled. Also confirm the cue is visible
without obscuring the landmark and observe frame rate in the normal 30 FPS
budget; this actor already ticks once per second and the presence check adds no
per-frame scan.
