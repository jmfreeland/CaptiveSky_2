# Landscape material lead (2026-09-28)

## Evidence inspected

- The source project at `D:/Projects - Athena/Unreal/CaptiveSky/CaptiveSky.uproject` declares Unreal Engine **5.7**. Its landscape candidates include `Content/Materials/M_Landscape.uasset` (32 KB), `Content/Materals/M_AutoLandscape.uasset` (161 KB), and `Content/Materals/MI_MountainRange.uasset` (41 KB), with a 6.7 MB `T_LandscapeNoise` texture and a 669 KB angle/grass texture.
- The active `CaptiveSky_2.uproject` declares Unreal Engine **5.8**. It already has `Content/Materials/M_Island_Textured_Auto.uasset` (164 KB) and `MI_Island_Landscape.uasset` (43 KB), plus `Content/Materals/M_AutoLandscape.uasset` (162 KB) and the same-named material functions/textures. SHA-256 checks show the two projects' `T_LandscapeNoise` and `T_Angle_Out_Grass` packages are byte-identical, while their `M_AutoLandscape` packages differ. `IslandEnvironmentSubsystem` wraps the landscape's authored material slots with transient dynamic instances so its existing `Ground Wetness` parameter can be driven and restored at end play.
- The older project's `Saved/AutoScreenshot.png` shows a more varied mountain-and-water scene than the active project's `Saved/AutoScreenshot.png`. This is a visual lead, **not** a controlled before/after: the screenshots show different scenes, and the old project is on a different engine minor version.
- The current saved Island viewpoint captures show an expansive brown/dark ground plane with sparse visible ground detail at the tested camera/time. They justify revisiting landscape presentation but do not by themselves prove the assigned material is the cause; lighting, landscape layer weights, texture streaming, or the capture state may also contribute.

## Decision

Do not copy or replace a binary material based on its asset name or screenshot alone. CaptiveSky_2 already contains a modified `M_AutoLandscape` and byte-identical copies of two old project's landscape textures, so the missing piece may be material assignment, layer setup, or a later material variant rather than absent source files. The open UnrealEditor process and Claude's worktree share the current `Content/` and derived data, so an asset migration could collide with active work. Keep the existing landscape material and runtime wetness hook intact until the graph and dependencies are inspected in the UE 5.8.3 editor.

## Next safe step

When the shared editor is available for coordinated use, first inspect the saved Island's assigned material and layer weights and compare them with the existing `M_Island_Textured_Auto`, `MI_Island_Landscape`, and `M_AutoLandscape` assets. Then open the older `M_Landscape` and `M_AutoLandscape` to compare parent graphs, layer-info requirements, and runtime-virtual-texture use. If a transfer or reassignment is worthwhile, duplicate to a new path in `CaptiveSky_2` rather than overwriting an existing package, migrate dependencies through the editor, assign it only to a reversible test copy/map first, and verify that `Ground Wetness` still drives and restores correctly. Capture the same Island viewpoint under matched lighting before accepting the change.

No Unreal assets were modified during this audit.
