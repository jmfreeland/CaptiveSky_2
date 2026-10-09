# Minnow surface ripple runtime validation (2026-10-09)

## Result

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

Evidence logs: [`Codex_RippleTintVisualWarmDDC_Game_20261009.log`](../../Saved/Playtests/Codex_RippleTintVisual_20261009/Codex_RippleTintVisualWarmDDC_Game_20261009.log), [`Codex_RippleTintVisualPolish_Game_20261009.log`](../../Saved/Playtests/Codex_RippleTintVisualPolish_20261009/Codex_RippleTintVisualPolish_Game_20261009.log), and [`Codex_RippleTint_VisualPolish_Automation_20261009.log`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Logs/Codex_RippleTint_VisualPolish_Automation_20261009.log).

## Next

Refine the surface response from this rendered evidence, then profile a normal
moving Game/PIE frame rate separately—the movement probe establishes the
interaction, not the project's 30 fps performance target. Re-render both the
mid-pulse and fade frames after the material change. Preserve the existing
45-second play and zero-request safeguards.
