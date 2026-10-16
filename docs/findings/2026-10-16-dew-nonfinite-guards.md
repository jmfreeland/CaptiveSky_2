# Dew effect: non-finite input guard (2026-10-16)

`ComputeDewStrength` now fails closed when any sampled weather or clock input is
NaN or infinite. `AIslandDewActor` also converts invalid strength to zero before
updating its material/instance pool, rejects non-finite requested counts, and
keeps invalid distances from producing invalid glint scales. This prevents a
transient bad environmental sample from contaminating the morning visual effect.

## Verification

- The edited `IslandDew.cpp` and `IslandDewTests.cpp` compiled successfully.
- The full UE 5.8.3 scratch editor target linked successfully (51 actions) against
  the current shared source, with its output isolated under
  `Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Binaries`.
- A direct main-project link could not replace
  `Binaries/Win64/UnrealEditor-CaptiveSky_2.dll`: Windows reported that
  `UnrealEditor.exe` still holds it. I left that process untouched, in keeping
  with the user's prior choice to leave the editor running.
- `CaptiveSky2.Agent.IslandDew` and `CaptiveSky2.Agent.IslandDewRuntime` both
  passed in the scratch editor under NullRHI, with Python and agent thinking
  disabled and model requests capped at zero. The log is
  `Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_IslandDew_NonFinite_20261016.log`.
- The main project DLL remains held by `UnrealEditor.exe`; only the isolated
  scratch target was linked. No saved Island state was loaded or changed.

The change does not alter weather thresholds, appearance, ecology timing, or
resident behavior; it only makes invalid input deterministic and inert.
