# Rendered PIE memory-pressure preflight — 2026-11-01

## Run and observations

The actual `D:\Projects - Athena\Unreal\CaptiveSky_2` project was reopened in
UE 5.8.3 after the user confirmed the map was saved. Startup completed, the
Island map check reported zero errors and zero warnings, and the fresh log
loaded `bEnableNaniteFoliage=1` from project configuration.

Before PIE, `nvidia-smi` reported 6,560 MiB free on the 12,282 MiB RTX 4080
Laptop GPU (0% utilization, 59 °C). PIE copied the Island into
`/Game/Maps/UEDPIE_0_Island` and reached `Server logged in` in 13.0 seconds.
Within the first rendered preview, whole-device free GPU memory fell to 483
MiB (51% utilization, 70 °C). Unreal then displayed a system Memory Pressure
Warning. PIE was stopped after 35.8 real seconds; the session log reports zero
model requests. After teardown, GPU free memory recovered to 2,898 MiB, while
Windows reported 1.90 GiB free physical RAM. These are point samples, not a
controlled peak-memory or frame-time profile.

The preview is not visual evidence for tree diversity, natural placement,
water collision, or the updated worktree's A/B canopy code: the editor camera
still faced offshore, and SHA-256 comparisons confirmed that the D: project's
`IslandWeather.cpp` and `IslandOceanSubsystem.cpp` differ from this Codex
worktree. Do not attribute this run to the source at the current worktree
revision. A separate log warning about
`M_Island_Textured_Wet: Requesting an invalid TextureIndex! (21 / 20)` also
reappeared; prior investigation shows this concerns an invalid texture-
collection index and does not establish the cause of memory pressure.

## Decision

Do not extend this editor configuration into a 15–30 minute rendered feedback
run. Idle GPU headroom alone was not a sufficient safety gate: the first PIE
world exhausted nearly all remaining dedicated GPU memory and triggered a
system pressure notification. The `Start-Spectator.ps1` launcher now has an
opt-in Windows RAM preflight and a bounded two-low-sample monitor alongside its
existing GPU guard; memory-query failure fails closed, and a monitor only stops
the Game process tree it launched. Before another rendered run, close other
heavy applications and require both guards with isolated time/request caps.
Keep the existing 30-FPS floor and stop on a memory warning. Character feedback
can continue in an isolated NullRHI run only after the known startup failure is
addressed; this PIE session produced no character feedback.

The new PowerShell system-memory boundary/monitor tests passed, as did the
existing GPU-memory tests and `Start-Spectator.ps1` parser check. Running the
new launcher preflight against the current machine rejected the launch at
2,269 MiB free RAM and confirmed no rendered process was started. The Python
static launcher-contract test could not be run because the only `python.exe`
on PATH is an inaccessible Windows Store alias. These script guards protect
launcher-started spectator runs; they do not intercept a user pressing Play in
the Unreal editor, so manual PIE still needs live resource observation.

The present run establishes a concrete runtime resource constraint, not a
rendering regression, a vegetation diagnosis, or a fix. Do not reduce canopy
counts, disable Nanite Foliage, or change landscape materials on this evidence
alone.

## Post-teardown retention sample (04:27 UTC)

The editor had returned to edit mode and remained responsive, but memory had
not recovered to the pre-PIE baseline. UnrealEditor PID 36236 reported a 15.0
GiB working set and 23.7 GiB private bytes. Windows showed 2,258 MiB of free
physical RAM; the GPU showed 9,098 / 12,282 MiB used (2,900 MiB free, 40%
utilization, 72 °C). These are process/device snapshots, not proof that all
retained memory is leakable or belongs to Island content. They do show that a
second rendered run should wait until RAM and GPU headroom recover and should
capture a baseline before pressing Play.
