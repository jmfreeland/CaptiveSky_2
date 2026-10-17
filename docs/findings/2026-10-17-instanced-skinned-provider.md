# Instanced skinned Megaplant provider — 2026-10-17

`AIslandInstancedSkinnedFoliage` exposed a skeletal mesh and transforms, then
added every instance with animation index zero, but it had no way to assign the
UE 5.8 `UTransformProviderData` required to drive those instances. The host now
exposes an optional editable provider and passes it to
`UInstancedSkinnedMeshComponent::SetTransformProvider` whenever it rebuilds.
The provider is cleared too when the property is unset, so repeated rebuilds
cannot leave a stale animation source attached.

Added `CaptiveSky2.Agent.InstancedSkinnedFoliageProvider` to verify provider
assignment and clearing through the host API. The current-source UE 5.8 editor
target built successfully in the isolated
`Saved/CompileScratch/Codex_WrackCrabIsolated_20261017/Project` project (6
actions, including UHT, the wrapper, its test translation unit, and module link).
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
