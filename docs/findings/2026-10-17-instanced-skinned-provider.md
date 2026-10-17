# Instanced skinned Megaplant provider — 2026-10-17

`AIslandInstancedSkinnedFoliage` exposes an optional editable
`UTransformProviderData` and passes it to
`UInstancedSkinnedMeshComponent::SetTransformProvider` whenever it rebuilds.
When a mesh has enabled UE 5.8 `UDynamicWindSkeletalData` and the user has not
supplied another provider, the host now creates a transient `UDynamicWindData`
and binds it automatically. Clearing an explicit provider clears the component
provider too, so repeated rebuilds cannot leave a stale animation source
attached. `DynamicWind` is declared as a module dependency and an enabled
project plugin.

Added `CaptiveSky2.Agent.InstancedSkinnedFoliageProvider` to verify provider
assignment and clearing and, when the installed European Beech asset and its
wind profile are available/enabled, automatic provider selection. The
current-source UE 5.8 editor target built successfully in the isolated
`Saved/CompileScratch/Codex_WrackCrabIsolated_20261017/Project` project (172
actions including UHT, both changed translation units, and module link). The
first build exposed a missing direct `DynamicWind` plugin entry in the
scratch descriptor; after syncing it with the main project, UBT regenerated the
makefile and reported the target up to date with no plugin dependency warning.
The main editor's loaded DLL was not replaced.

The automation test has not run. Launching it from the existing
`Codex_VisitorResonance_20261017` scratch shell (where other NullRHI tests had
passed, with project plugins enabled, thinking disabled, and model requests
set to zero) still crashed before the project log or test discovery. The new
crash report records a null-address access violation at `SecondsSinceStart=0`
with the same Core stack hash (`F405610C...`); see
[`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md).
Thus the build proves API/UHT/link compatibility, not runtime provider behavior.
No Megaplant asset or Island map was changed, and no animated appearance or
performance improvement has yet been verified in-world. Next, run the focused
test through a known-good editor-driven validation path, then supply a real
compatible provider and review a candidate Megaplant in a controlled scene
before considering placement on the Island. Do not repeat the same commandlet
launch until a meaningfully different editor-driven route is available or the
startup fault is diagnosed.
