# Minnow surface ripple runtime validation (2026-10-09)

## Result

The close, low-angle capture exposed the remaining weakness: the single dark
ring still read as a drawn outline at the moment of the startle. The latest
iteration adds two softer trailing crests to the material ripple and reduces
the transient ring tint again. In the mid-pulse close frame the cue is now
subdued and sits within the water; once it fades, the broader surface wake is
more legible. The peak cue still has a basin-shaped circular footprint, so the
water distortion—not the ring—is the part to keep developing.

![Close mid-pulse minnow startle](../../Saved/Playtests/Codex_RippleWakeVisual_20261009/ScreenshotsCloseFinal/000_MinnowStartleMidPulse.png)

![Close pool after the cue fades](../../Saved/Playtests/Codex_RippleWakeVisual_20261009/ScreenshotsCloseFinal/001_Ripple__Close.png)

The first D3D12 Game run rendered the pool and transient response after the
shader-preparation overlay cleared. Its bright, clean cyan circle was readable
but felt more like a graphic marker than water. That evidence drove a small
source-only art iteration: widen the band, add restrained radial irregularity,
and lower/desaturate its teal emissive color. The second Game capture is more
subtle and blends better with the water, although the broad camera still reads
the cue as a circular ring; a future pass should make the actual surface
distortion carry more of the organic character.

![Minnow startle ripple at mid-pulse](../../Saved/Playtests/Codex_RippleTintVisual_20261009/ScreenshotsWarmDDC/000_MinnowStartleMidPulse.png)

![Same pool later, after the first cue fades](../../Saved/Playtests/Codex_RippleTintVisual_20261009/ScreenshotsWarmDDC/004_Mid__Distance.png)

![Revised mid-pulse response](../../Saved/Playtests/Codex_RippleTintVisualPolish_20261009/Screenshots/000_MinnowStartleMidPulse.png)

![Revised pool after the cue fades](../../Saved/Playtests/Codex_RippleTintVisualPolish_20261009/Screenshots/004_Mid__Distance.png)

## Runtime evidence

- UE 5.8.3 standalone Game, D3D12, 1280×720; warmed local DDC.
- Gameplay remained bounded to 45 seconds maximum with agent thinking disabled;
  the session ended after 31.5 real seconds and recorded **0 model requests**.
- Raven's forced-curiosity flyby completed a 1,937 cm flight in 4.6 simulated
  seconds. The minnow probe observed five bodies and a maximum 107 cm school
  centroid shift from the cue-time baseline. The isolated probe reported
  success.
- After the visual polish, `CaptiveSky2.Agent.TidepoolMinnows` passed and the
  second bounded Game run also exited normally: a 1,072 cm flight in 3.2
  simulated seconds, 26 post-startle samples with 103 cm maximum school shift,
  and 0 model requests in 26.7 real seconds. That run had one low-flyby sample;
  it is evidence the cue and movement work together, not a broad ecology sample.
- The trailing-wave iteration built successfully; `CaptiveSky2.Agent.IslandTideglass`
  and `CaptiveSky2.Agent.TidepoolMinnows` both passed. The final close-view Game
  run ended normally after 27.3 real seconds with 0 model requests. Raven flew
  1,504 cm over 3.7 simulated seconds; the probe captured four low-flyby samples,
  51 cm maximum school-centroid shift in the flyby envelope, and 26 post-startle
  samples with a 106 cm maximum shift from the cue-time baseline. This validates
  the bounded behavior interaction, not frame-rate performance.
- The generated `M_TideglassPool_RippleWake.uasset` is local/ignored under the
  repository's existing `Content/` convention, not included in the Git push.
  Recreate it with `Scripts/Create-TideglassPoolMaterial.py` and
  `CAPTIVESKY_TIDEGLASS_MATERIAL_NAME=M_TideglassPool_RippleWake`; the prior
  ripple asset remains available as a local fallback.
- The first-use Game capture logged 450 PSO creation hitches. This was a cold
  startup sample, not a warmed performance profile; the 30 fps target remains
  unverified and should be measured separately with warm caches.
- No `dotnet.exe` process or matching recent `.NET Runtime` / `Application Error`
  event was found during the standalone Game runs. Unreal exited normally, so
  those runs do not explain the separate dialog; the sandboxed UBT exception is
  recorded in the companion findings note.
- The first standalone launch took about 112 seconds to reach a useful
  in-world capture. With shader/assets cached, the revised run reached its
  first screenshot in about 48 seconds. The Game log also reports a missing
  `/Script/WaterEditor` import for the cloned map; the pool nevertheless
  rendered. Treat that warning as a scratch project/plugin issue to resolve if
  it recurs in the main editor.

Evidence logs: [`Codex_RippleWakeCloseFinal_Game_20261009.log`](../../Saved/Playtests/Codex_RippleWakeVisual_20261009/Codex_RippleWakeCloseFinal_Game_20261009.log), [`Codex_RippleWake_FinalPolish_Build_20261009.log`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Logs/Codex_RippleWake_FinalPolish_Build_20261009.log), [`Codex_RippleWake_Tideglass_Automation_20261009.log`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Logs/Codex_RippleWake_Tideglass_Automation_20261009.log), and [`Codex_RippleWake_TidepoolMinnows_20261009.log`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Logs/Codex_RippleWake_TidepoolMinnows_20261009.log). Earlier visual captures remain linked above.

## Next

Profile a normal moving Game/PIE frame rate separately—the movement probe
establishes the interaction, not the project's 30 fps performance target. Then
continue refining the surface response if a warmed close capture still reads as
a ring. Preserve the existing 45-second play and zero-request safeguards.
