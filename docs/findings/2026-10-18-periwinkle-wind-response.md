# Periwinkle response to open-air wind — 2026-10-18

Tidepool periwinkles now ease their eyestalks and grazing activity inward as
the Island's ambient wind rises. A normal breeze produces only a slight
response; the tuck saturates at 0.38 during the strongest modeled wind so the
snail never appears fully sealed by wind alone. Tide exposure, heavy rain,
and close visitors retain their existing stronger behavior. Shell wetness is
still driven only by tide and rain, so wind does not make a dry shell look wet.

The response uses the weather actor's deterministic broad wind sample at the
snail's home point and current world time. It adds no per-snail shelter trace,
LLM call, persistence, or additional tick; movement, stalk pose, and grazing
continue to share the existing 0.2-second actor tick.

`CaptiveSky2.Agent.TidepoolPeriwinkleWindExposure` covers calm, ordinary breeze,
storm-strength, saturation, negative, and non-finite inputs. A focused runtime
automation test translation unit compiled and linked successfully in the UE
5.8.3 editor target using the isolated
`Saved/CompileScratch/Codex_WrackCrabIsolated_20261017/Project` build project;
the main editor's loaded module was not replaced. The automation itself and a
Game-view inspection remain pending: the connected editor window has been
unresponsive, and the known commandlet startup crash has not been resolved.
Until those gates are available, this is a deterministic source/build change,
not a claim of visually verified animal behavior.
