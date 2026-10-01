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

The controlled wetness pairs and material graph trace are recorded below. Next, test a matched pair at a low, grazing sun angle; if specular-only wetness still reads poorly, prototype roughness/puddle response in a reversible copy. Keep the old `M_Landscape` and `M_AutoLandscape` as references, not assumed replacements.

## Wet-pool scale follow-up (2026-10-01)

A controlled noon comparison tested three capture-only pool scales against the original generated
wet graph. The broadest variant suppressed reflective speckle but nearly removed wet response; the
balanced variant also weakened the detailed ground shot. The selected 1,200 cm / 0.36 coverage /
6.5 sharpness setting retained broader connected pools in the Tideglass detail capture and reduced
the blue-reflection pixel proxy in four of five wider views. Those values are now the defaults in
`Scripts/Create-LandscapeWetMaterial.py`. The detailed metrics, output paths, and test caveats are in
[`2026-09-30-landscape-wetness.md`](2026-09-30-landscape-wetness.md#pool-scale-comparison-and-selected-builder-defaults-2026-10-01).

UE 5.8.3 rebuilt both generated wet assets and passed a fresh paired nine-view automation run via
the scratch project. Its `Content/` is junctioned to the main project, so the ignored wet parent and
wet instance were regenerated in the main Content tree too. The saved landscape assignment and
authored assets were not changed. The offscreen result is not yet a PIE or packaged-build validation;
inspect a real rain-to-dry transition in the user's editor before calling the wet-ground presentation
finished.

No Unreal assets were modified during the original 2026-09-28 audit. Later generated wet-graph and
capture-only assets are ignored by Git; the saved authored landscape materials and map remain unchanged.

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
baseline. The authored material remains `Ground Wetness = 0.15` when dry, rising to `1.0` when fully
wet; the close-up and graph trace that follow further investigate its effect.

## Close-range wetness and graph trace (2026-09-30)

Added the fixed `02a_TideglassGroundDetail` camera to `Config/IslandViewpoints.json` and captured
another same-session dry/fully-wet pair, this time looking down across the detailed ground around
Tideglass Pool. The paired frames are
`Saved/Viewpoints/2026-09-30_115447_h12.0_authored_dry/02a_TideglassGroundDetail.png` and
`Saved/Viewpoints/2026-09-30_115447_h12.0_fully_wet/02a_TideglassGroundDetail.png`. The close-up
still has no readily visible change at noon. A second matched pair at 17:00 also appeared nearly
identical:
`Saved/Viewpoints/2026-09-30_122416_h17.0_authored_dry/02a_TideglassGroundDetail.png` and
`Saved/Viewpoints/2026-09-30_122416_h17.0_fully_wet/02a_TideglassGroundDetail.png`. That sunset
view is very dark, however, so it is weak evidence about grazing-angle specular rather than a
conclusive lighting test. Both paired automation runs passed without starting play or writing world
state.

A read-only UE Python graph inspection then traced the `Ground Wetness` scalar in
`M_Island_Textured_Auto` into an `MF_CreateLayer` call. That call maps the scalar to the function's
`Specular` input; inside `MF_CreateLayer`, that input feeds the `Specular` input of
`MakeMaterialAttributes`, which is returned as the layer material attributes. The inspected wetness
node therefore changes specular response, not the layer's roughness or a puddle mask. This narrow,
specular-only path plausibly explains why changing 0.15 to 1.0 is imperceptible in the noon pairs;
it does not prove there is no lighting-angle-dependent difference.

The successful graph inspection used UE 5.8.3 with the project's required
`-shaderworkingdir=<project>/Saved/ShaderWorking`. A prior one-off launch omitted that flag and
crashed before Python ran when UE could not create a transfer file under the default user shader
directory. The read-only diagnostic is `Scripts/Inspect-LandscapeWetness.py`. No material asset was
modified or saved. The graph dump is in
`%TEMP%/Codex_InspectLandscapeWetness_MFCreateLayer_20260930.log`.

The next useful step is a reversible material prototype that also lowers roughness and/or drives a
restrained puddle mask from wetness; don't replace the authored material in place. Recheck runtime
drive-and-restore behavior and verify the prototype in PIE before considering migration. `Content/`
assets are outside this source audit and were not changed.

## Puddle-branch prototype (2026-09-30)

UE's read-only material inspection found an existing `MF_Puddles` branch gated by the static switch
`Add Puddles`, but the assigned `MI_Island_Landscape` has the switch disabled. Its parent,
`M_Island_Textured_Auto`, also exposes `Puddle Depth` (authored as 3.0) and `Puddle Clarity` (0.75).
Created a separate ignored Content asset, `/Game/Materials/MI_Island_Landscape_WetPrototype`, with
only `Add Puddles` enabled; the assigned instance and Island map were not modified. The reproducible
editor Python helper is `Scripts/Create-LandscapePuddlePrototype.py`.

The paired, same-session noon capture at
`Saved/Viewpoints/2026-09-30_135732_h12.0_authored_dry/02a_TideglassGroundDetail.png` and
`Saved/Viewpoints/2026-09-30_135732_h12.0_fully_wet/02a_TideglassGroundDetail.png` used that
prototype with puddle depth 0 when dry and 3 when fully wet. `CaptiveSky2.Visual.Viewpoints` passed,
including the exact landscape material restoration check, with no play session or world-state write.
The two frames differ slightly at pixel level but show no convincing visible puddle response from
this camera. This is evidence that enabling the existing branch alone is not a sufficient visual
fix; inspect its mask/coordinate inputs and tune it in a reversible copy before considering any
runtime integration. The prototype `.uasset` is ignored by Git, so the helper can recreate it but
the generated asset itself is not part of a source commit.

The preview harness now snapshots every landscape component's material before assigning any
preview material. `ULandscapeComponent::SetMaterial` can affect later component reads through the
proxy's shared landscape-material override, so interleaving snapshot and assignment made restoration
order-dependent. The capture now validates exact restoration after a successful render.

### Puddle mask scale/contrast follow-up (2026-09-30)

Extended the read-only UE inspection to identify the `MF_Puddles` texture sample: it uses
`/Game/Materals/Textures/T_LandscapeNoise.T_LandscapeNoise` with landscape-layer coordinates divided
by `Puddle Size`, then raises that sample to `Puddle Constrain`, multiplies by `Puddle Depth`, and
saturates the result. The assigned instance's effective values are `Puddle Size=200`,
`Puddle Depth=3`, `Puddle Clarity=0.75`, and notably `Puddle Constrain=10`; the function's displayed
defaults alone were therefore misleading about the actual mask.

Created two additional ignored, isolated copies to test the hypothesis: one used size 500 / constrain
1 / clarity 0.5, and a deliberately exaggerated visibility probe used size 25 / depth 20 / constrain
1 / clarity 0.1. UE readbacks confirmed those overrides and `Add Puddles=True`; the authored instance
still reports `Add Puddles=False`. Both matched noon captures passed the restoration assertion. The
exaggerated wet frame remains effectively pixel-identical to the milder puddle prototypes and shows
no convincing pooled-water shapes, while the within-run dry/wet pair does change slightly. Thus the
branch is not yet a useful visual improvement even under extreme scalar tuning; do not wire it into
the environment subsystem. Next inspect the parent material's compiled/static-switch path and the
actual puddle function output in the material editor, then choose a new mask/roughness approach if
that branch proves inert. Prototype assets live in ignored `Content/` and are reproducible with
`Scripts/Create-LandscapePuddlePrototype.py`, `Scripts/Create-LandscapePuddleScalePrototype.py`,
and `Scripts/Create-LandscapePuddleVisibilityProbe.py`; none is assigned to the map.

### Puddle function wiring recheck (2026-09-30)

A fresh read-only UE 5.8.3 graph dump confirms `MF_Puddles` is connected rather than dangling.
Its `Material with Puddles` output is selected by the `Add Puddles` static switch, whose two
inputs are the modified `SetMaterialAttributes` result and a pass-through of the original
attributes. The assigned landscape instance has that switch false; the separate visibility
probes override it to true. The mask path is `T_LandscapeNoise` sampled using landscape-layer
coordinates divided by `Puddle Size`, then `pow(noise, Puddle Constrain) * Puddle Depth`,
saturated. That mask drives attribute lerps for base color, normal, roughness and specular;
`Puddle Clarity` scales the base-color contribution. The graph therefore contains the
expected visual response path, but the earlier matched captures still showed no convincing
puddles, even with the deliberately exaggerated probe parameters. The weak capture is not
evidence that the branch is disconnected.

The useful next diagnostic is to preview the raw mask and each modified attribute separately
at the close Tideglass camera; this will distinguish a mask-distribution issue from an
attribute/blend issue before any runtime integration. No authored material, instance or map
was modified or saved. Graph output: `Saved/Logs/Codex_LandscapeFunctionAudit_20260930.log`.

That inspection log also contains `LogPython: Warning` lines from an experimental probe of a
generic `inputs` property that UE does not expose on these expressions. The existing
`MaterialEditingLibrary` graph queries succeeded and produced the trace above. The temporary
probe was removed from `Scripts/Inspect-LandscapeWetness.py`; no project gameplay or asset
failure was involved.

### Puddle mask material preview attempt (2026-09-30)

Added `Scripts/Create-LandscapePuddleMaskDebug.py` to create a distinct, ignored material at
`/Game/Materials/M_Island_PuddleMaskDebug`. The first version was generated successfully but
the 12:00 close-up capture displayed only the landscape checkerboard fallback; UE also logged a
handled ensure in `LandscapeRender.cpp` while updating landscape material instances, and
`CaptiveSky2.Visual.Viewpoints` failed. The output is
`Saved/Viewpoints/2026-09-30_202252_h12.0_fully_wet/02a_TideglassGroundDetail.png`; the
paired dry/wet captures do not validate mask appearance. The authored landscape material and
Island map assignment were not changed.

The capture failure suggests a newly created generic material does not retain the working
landscape setup needed by this map. A second builder attempt duplicated
`/Game/Materials/M_Island_Textured_Auto` to
`/Game/Materials/M_Island_PuddleMaskDebug_Landscape`, but clearing its expression graph caused
`Landscape Physical Material Output` compile errors due to missing inputs. The current builder
keeps the copied graph and routes only the diagnostic Base Color and Emissive outputs around it;
this revision still needs a real-RHI validation. UE 5.8.3 also reported repeated Zen DDC
connection retries during editor startup. The capture helper now
supports an explicit `-LogPath` and uses a process-specific `%TEMP%` log by default: its old
shared name collided with a concurrent run from the Claude worktree, making combined log text
ambiguous. No gameplay or world-state changes were made, and no authored material or map asset
was saved.

### Wet-parent capture visual check (2026-09-30)

The 12:00 authored-dry/fully-wet pair at
`Saved/Viewpoints/2026-09-30_204634_h12.0_{authored_dry,fully_wet}/` predates the component-MIC
recache fix. Its mean absolute RGB difference is only `2.287/255`, as expected when both passes
still rendered the old graph. It is not evidence against the current wet-parent response; see
[`2026-09-30-landscape-wetness.md`](2026-09-30-landscape-wetness.md) for the later recached capture,
which measured mean RGB `[72.0, 56.9, 4.5]` dry versus `[39.7, 32.1, 10.0]` wet.

### Puddle-mask visualization and parent-only capture fix (2026-09-30)

The first generic debug material at `/Game/Materials/M_Island_PuddleMaskDebug` showed the
landscape checkerboard and caused a handled `LandscapeRender.cpp` ensure. Duplicating
`M_Island_Textured_Auto` retained the landscape setup, but deleting its graph failed compilation
because `Landscape Physical Material Output` had missing inputs. Keeping the graph allowed a
separate material to compile, but parent-only viewpoint commands still displayed the original
colored landscape.

The cause was in `IslandViewpointCaptureTest`: it parsed `-ViewpointLandscapeParent` only inside
the wetness-preview conditional. A capture with `-LandscapeParent` alone therefore passed while
never swapping the parent. The automation source now applies any requested parent swap
independently of the optional wetness-preview branch; that C++ change still needs a build and a
parent-only verification run.

With the wetness branch explicitly activated (`-LandscapeWetness 1`), the log confirmed the
in-memory swap to `/Game/Materials/M_Island_PuddleMaskDebug_AttrOutput` and transient wetness on
4,096 landscape slots. The test restored state and reported success, but its capture
`Saved/Viewpoints/2026-09-30_220218_h12.0/02a_TideglassGroundDetail.png` was nearly black: mean RGB
`2.06/255`, maximum channel value `12`, and 25.71% of pixels above `3/255`. The diagnostic was
still using the authored mask values (`Puddle Size=200`, `Puddle Depth=3`, `Puddle Constrain=10`),
so this indicates that this mask path produces very little signal at those values; it does not
yet distinguish the threshold distribution from a graph-output issue.

The first exaggerated capture looked yellow because scalar-to-vector promotion did not populate
all material-attribute color channels. An `AppendVector` attempt then failed UE compilation because
the landscape mask was carried as a two-component value. The final graph explicitly masks only R
(with G/B/A disabled), then appends that scalar into an RGB triplet. UE 5.8.3 compiled and saved
`/Game/Materials/M_Island_PuddleMaskDebug_ExaggeratedRGB` successfully.

The final close-up capture is
`Saved/Viewpoints/2026-09-30_230104_h12.0/02a_TideglassGroundDetail.png`; its run log is
`Saved/Logs/Codex_PuddleMaskDebug_ExaggeratedRGB_FinalCapture_20260930.log`. The viewpoint test
passed, logged the graph swap and transient wetness on 4,096 slots, and restored the landscape
state. Pixel statistics are RGB mean `[158.24, 158.43, 158.42]`, channel standard deviation
`[10.18, 10.12, 10.13]`, and range `123–175` in every channel (261 unique RGB values). This
confirms the replicated raw mask reaches the landscape in neutral grayscale, but the viewed signal
is still low-contrast noise rather than convincing pooled-water shapes. It is not yet a visual
validation of the authored `MF_Puddles` blend or grounds for enabling its static switch.

The capture helper now routes DDC to the ignored project-local `Saved/LocalDDC` instead of forcing
an in-memory cache. UE confirmed that path is writable; subsequent capture startups reused its
shader data instead of recompiling the full engine cache. The machine-wide cache remains read-only
in this environment. All debug materials are separate ignored assets; the authored landscape
material, instance, map assignment and world state remain unchanged.

The parent-only fix in `IslandViewpointCaptureTest` was compiled by invoking MSVC with Unreal's
generated response file, then linking the updated test object into an isolated scratch editor module
(the normal `Build.bat`/UBT launcher still idles). The scratch `UnrealEditor-Cmd` run used
`-ViewpointLandscapeParent` without `-LandscapeWetness`, exited 0, logged the in-memory graph swap,
and wrote `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-09-30_232349_h12.0/02a_TideglassGroundDetail.png`.
This verifies the parent-only code path; it does not improve the mask, which remains low contrast.

Overnight validation triage: `Codex_FullAgentSuite_20260930.log` records 31 successful tests and
zero failures. The two 20:52/20:54 standalone wetness crashes are old reports from before commit
`89acbe6`, which added the `GIsEditor` guard around the editor-only `SetParentEditorOnly` call.
The 20:22 landscape ensure is from the earlier generic debug material with no usable landscape
material interfaces; that attempt was abandoned in favor of retaining the authored landscape graph.
Separate startup fatals in the crash archive cite inaccessible shader-temp or read-only DDC paths,
not CaptiveSky runtime behavior. The first scratch retry in this session also failed because it
omitted the writable local DDC argument; the corrected retry above passed.

### Raw noise and thresholded mask calibration (2026-09-30)

Added three reproducible UE Python helpers: `Scripts/Create-LandscapePuddleNoiseDebug.py` creates
a raw-R preview; `Scripts/Create-LandscapePuddleContrastDebug.py` creates a high-contrast
thresholded preview; `Scripts/Set-LandscapePuddleContrastThreshold.py` retunes only that generated
diagnostic's threshold. All generated materials duplicate the functioning landscape parent to keep
its landscape physical-material setup, and none is assigned to or saved into the Island map.

The raw-noise close-up at 200 cm tile size produced grayscale capture values with mean `203.23/255`,
standard deviation `8.18`, and range `175–224`. A threshold-remapped preview made the spatial mask
legible: `saturate((R - threshold) * 10 + 0.5)` gave `2.35%` of pixels above `128/255` at threshold
`0.60`, versus `10.19%` at `0.55`. Captures are
`Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-09-30_235109_h12.0/02a_TideglassGroundDetail.png`
and `.../2026-09-30_235407_h12.0/02a_TideglassGroundDetail.png`, respectively. Both UE 5.8.3
material-creation runs and the viewpoint automation exited 0; transient parent/wetness previews
left the authored landscape parent, instance, map and world state untouched.

This is a diagnostic mask, not a proposed final puddle material: the hard threshold shows where
coverage may fall but does not validate the existing `MF_Puddles` base-color, normal, roughness,
or specular blends under lighting. Next compare those modified attributes in the same close view
before deciding whether the existing exponent-based mask should be tuned or replaced in a separate
prototype. Do not enable the authored static switch based on these threshold images alone.

### Baked-parent puddle preview correction (2026-10-01)

The earlier `-ViewpointLandscapePuddlePreview -ViewpointLandscapeMaterial` implementation assigned
MIDs with `ULandscapeComponent::SetMaterial`, which cannot reach this map's baked per-component
landscape material instances. Two matched A/B probes of the old puddle instance and wet-material
instance therefore both measured the same low signal as the dry map (mean absolute RGB difference
`2.287/255`). The preview command now swaps the selected preview graph onto the landscape's assigned
MIC, recaches the baked component instances and drives its material-collection wetness from 0 to 1.
Parent restoration now recaches those instances and their static permutations as well. The CLI
example uses the working wet graph `/Game/Materials/M_Island_Textured_Wet`; no authored Content asset
or map was edited or saved.

In UE 5.8.3, the repaired puddle command and the direct parent command both pass
`CaptiveSky2.Visual.Viewpoints` and restore the authored parent. The wet graph's paired close-up
measures dry RGB `[72.00, 56.87, 4.54]` versus wet `[39.71, 32.06, 10.04]`, with mean absolute
pixel difference `24.671/255` (all pixels change by more than 5 levels). The wet capture visibly
shows sky-reflecting pools while the surrounding ground stays dark and slopes stay mostly dry.
Output: `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-10-01_003128_h12.0_{authored_dry,fully_wet}/02a_TideglassGroundDetail.png`;
log: `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Logs/Codex_PuddleCommandFixed_20261001.log`.
The normal UBT launcher remained idle for three minutes and was cancelled through its own session;
the changed capture test was compiled with UE's generated MSVC response file and linked only into
the ignored scratch editor module for this validation. The authored landscape assignment and
weather/world state were not persisted.

The broad `RunTests CaptiveSky2` verification ran 39 tests but is not a clean suite result:
37 passed; `IslandInnkeeperSpawn` failed only its identity/personality-file assertions because the
isolated scratch project does not contain the main checkout's untracked `Agents/Agent_Innkeeper_01`
documents, and `LLMProviderLive` could not connect to its configured OpenAI-compatible endpoint.
The latter is a live-provider test and should be excluded from future offline suite runs. No
unhandled exception or fatal crash appeared in this run; the `IslandInnkeeperSpawn` teardown logged
two `World has no context` actor-destroy warnings. The focused visual capture passed independently.
