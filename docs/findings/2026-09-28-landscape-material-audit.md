# Landscape material lead (2026-09-28)

## Evidence inspected

- The source project at `D:/Projects - Athena/Unreal/CaptiveSky/CaptiveSky.uproject` declares Unreal Engine **5.7**. Its landscape candidates include `Content/Materials/M_Landscape.uasset` (32 KB), `Content/Materals/M_AutoLandscape.uasset` (161 KB), and `Content/Materals/MI_MountainRange.uasset` (41 KB), with a 6.7 MB `T_LandscapeNoise` texture and a 669 KB angle/grass texture.
- The active `CaptiveSky_2.uproject` declares Unreal Engine **5.8**. It already has `Content/Materials/M_Island_Textured_Auto.uasset` (164 KB) and `MI_Island_Landscape.uasset` (43 KB), plus `Content/Materals/M_AutoLandscape.uasset` (162 KB) and the same-named material functions/textures. SHA-256 checks show the two projects' `T_LandscapeNoise` and `T_Angle_Out_Grass` packages are byte-identical, while their `M_AutoLandscape` packages differ. `IslandEnvironmentSubsystem` wraps the landscape's authored material slots with transient dynamic instances so its existing `Ground Wetness` parameter can be driven and restored at end play.
- A preliminary package-string scan finds the old `/Game/Materals/MI_MountainRange` points to `/Game/Materals/M_AutoLandscape`, while the active `/Game/Materials/MI_Island_Landscape` points to `/Game/Materials/M_Island_Textured_Auto`. Both instances reference the same groups of Megascans asphalt, snow, moss, cliff and rocky-ground textures; the current parent in turn references the shared angle/grass and landscape-noise textures and functions. This scan does not replace Unreal's Asset Registry or editor dependency validation.
- `CaptiveSky2.Agent.IslandEnvironment` loads `/Game/Materials/MI_Island_Landscape` and verifies that a transient dynamic instance accepts the `Ground Wetness` scalar. This proves the material parameter contract, not that the saved Island renders visible wetness, that its landscape layer assignments are populated, or that it looks correct under play lighting.
- The older project's `Saved/AutoScreenshot.png` shows a more varied mountain-and-water scene than the active project's `Saved/AutoScreenshot.png`. This is a visual lead, **not** a controlled before/after: the screenshots show different scenes, and the old project is on a different engine minor version.
- The current saved Island viewpoint captures show an expansive brown/dark ground plane with sparse visible ground detail at the tested camera/time. They justify revisiting landscape presentation but do not by themselves prove the assigned material is the cause; lighting, landscape layer weights, texture streaming, or the capture state may also contribute.

## Decision

Do not copy or replace a binary material based on its asset name or screenshot alone. CaptiveSky_2 already has a textured automatic parent assigned through `MI_Island_Landscape`, a separate modified `M_AutoLandscape`, the source textures, and a passing wetness-parameter contract. The saved-map audit below confirms the expected instance occupies every landscape material slot and that no painted layer allocations are present. The remaining uncertainty is how the configured auto-material blends and renders under matched dry/wet and lighting conditions. Keep assets unchanged until those inputs identify a specific cause.

## Follow-up (2026-09-28)

- Captured a fresh h12 WindArch and h15 Tideglass frame with the no-play viewpoint script and optional ground-cover preview. The terrain still reads broad and muted at those fixed cameras, but that alone does not distinguish an assignment problem from the material's automatic blend, landscape scale, lighting, or layer/weight state.
- A controlled UE 5.8.3 golden-hour comparison (`-Hour 17 -Only WindArchOverlook -GroundCover -NoWorldState`) gives the WindArch a stronger sunset silhouette, while its frame still shows a large brown ground plane and blockout-scale foreground props. The same no-play Tideglass view at 08:00 confirms sparse transient grass placement around the pool, but not the in-play minnows, weather, or fireflies. These captures do not justify a landscape-asset swap; they make the visual target and editor-side assignment/layer inspection more concrete.
- The computer-use app inventory returned no desktop application windows, and no UnrealEditor process was running. No material, map, or other `Content/` asset was changed.
- UE 5.8.3 build passed after extracting the grounded-to-airborne approach preflight into a shared helper. The Island-aware `RavenPerch` test now confirms complete, speaking-range ground paths from Aster's spawn to both actual roost markers; this does not validate the landscape's visual appearance.

## Night visibility check (2026-09-29)

A matched 20:00 `ShoreApproach` capture showed that the prior moon intensity (`0.24`) and night
skylight floor (`0.36`) left nearly all terrain black at the project's fixed exposure. Raising them
only to `0.8` / `1.2` brightened the clouds but still left the ground unreadable. The current candidate
uses `1.5` moon intensity and a `2.5` skylight floor: the same no-play view now shows the textured
shore and ridge under a cool, dark-blue night. The first dedicated `StonesFirefly` capture still
read the insect as a tiny bluish speck, leading to a separate procedural glow follow-up below.

The final candidate compiled with UE 5.8.3 and rendered successfully at both `ShoreApproach` and
`StonesFirefly` using `-NoWorldState`. Captures: `Saved/Viewpoints/2026-09-29_104735_h20.0/01_ShoreApproach.png`
and `Saved/Viewpoints/2026-09-29_104848_h20.0/05_StonesFirefly.png`. No map or material asset was
changed. A later PIE check should confirm the same balance in the live viewport and on the user's
display before further tuning.

### Firefly visibility follow-up (2026-09-29)

The fixed `StonesFirefly` view places its route-side firefly about seven metres from the camera,
but the original lit placeholder was barely distinguishable against the ground. The firefly now
uses UE's built-in emissive mesh material with a per-instance yellow-green `Color`, a restrained
seven-centimetre body, and the existing pulsing local light. Rain, quiet observation, and Listening
Stones chimes still modulate its glow; population and roaming behavior are unchanged. This is a
procedural readability improvement, not a finished insect asset, and no project `Content/` material
or map was changed.

The UE 5.8.3 build and `CaptiveSky2.Agent.NightEcology` test passed. The matched no-play capture at
`Saved/Viewpoints/2026-09-29_112621_h20.0/05_StonesFirefly.png` shows a distinct green light point on
the shore. Individual wing detail is still not readable at this distance, and the ground remains
very dark. Confirm the effect in PIE and on the user's display before considering the visual task
complete.

## Next safe step

Compare the same Island location in a material preview and in-world under matched lighting, both dry and wet; include the runtime `Ground Wetness` drive-and-restore behavior. The audit found no painted layer allocations, so focus next on the automatic material's blend logic and current texture/scalar configuration. Only if this identifies a specific cause should a material be duplicated or migrated for a reversible test, with the wetness behavior revalidated. The old `M_Landscape` and `M_AutoLandscape` remain useful references, not assumed replacements.

No Unreal assets were modified during this audit.

## Saved-Island assignment audit (2026-09-30)

The read-only `CaptiveSky2.Agent.IslandLandscapeAssignment` test passed against the saved Island. It
found one editor map, one landscape actor, 4,096 landscape components and 4,096 assigned material
slots. Every slot resolves to `/Game/Materials/MI_Island_Landscape.MI_Island_Landscape`; there are no
alternate or null material assignments. No components have allocated painted weightmap layers.

The instance's parent is
`/Game/Materials/M_Island_Textured_Auto.M_Island_Textured_Auto`. Its overrides include Rocky Ground
for the ground layer, Mossy Grass for MidLow, Rocky Ground for MidHigh, Rock Cliff, Windswept Snow,
and Asphalt for roads. Among the notable scalar settings are Ground Wetness `0.15`, Ground AO
Intensity `0.3`, Near/Far Tiling Sizes `4`/`40`, and Blend Distance Start/Transition `1000`/`5000`.
The full parameter dump is in `Saved/Logs/Codex_LandscapeAudit_Automation_20260930.log`; the
successful UE 5.8.3 build log is `Saved/Logs/Codex_LandscapeAudit_Compile_20260930.log`.

This rules out a missing or mixed landscape-material assignment as the cause of the broad, muted
terrain in the recent no-play captures. The audit establishes configuration but not the exact visual
cause; material blend behavior and close-range dry/wet rendering still need inspection. No `Content/`
asset was changed. Use the transient paired-preview tooling below at a closer, lower camera angle,
then inspect material wiring before considering a reversible material-copy experiment.

## Transient wetness preview (2026-09-30)

`Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only WindArchOverlook -NoWorldState -CompareLandscapeWetness`
now captures the authored-dry value and fully wet value in the same editor-world session, using
transient material instances. Cleanup restores the original material parent and authored wetness;
UE creates per-component dynamic wrappers when the parent is reassigned, so the test validates that
equivalent state rather than pointer identity. The editor-only capture does not start play or write
world state. The UE 5.8.3 build and `CaptiveSky2.Visual.Viewpoints` run passed; the run prepared all
4,096 landscape material slots. Captures:

- `Saved/Viewpoints/2026-09-30_113633_h12.0_authored_dry/04_WindArchOverlook.png`
- `Saved/Viewpoints/2026-09-30_113633_h12.0_fully_wet/04_WindArchOverlook.png`

Build and automation logs are `Saved/Logs/Codex_LandscapeWetnessPair_Compile_20260930.log` and
`Saved/Logs/Codex_LandscapeWetnessPair_Automation_20260930.log`.

At this wide Wind Arch composition, the two states show no obvious terrain change. This is not enough
to conclude the parameter is disconnected: the view is dominated by a broad, distant surface and may
not resolve puddle/roughness detail. Earlier captures launched in separate editor processes also had
different cloud states, so they are not a valid visual comparison. The paired result is the useful
baseline; next inspect a closer, low-angle landscape view or the material's wetness wiring before
changing any asset. The authored material remains `Ground Wetness = 0.15` when dry, rising to `1.0`
when fully wet.
