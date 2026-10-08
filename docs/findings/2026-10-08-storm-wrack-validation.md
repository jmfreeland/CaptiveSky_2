# Storm wrack validation (2026-10-08)

`CaptiveSky2.Agent.IslandWrack` passed in an isolated UE 5.8.3 scratch project
using the same `IslandWrack.cpp`, `IslandWrack.h`, and `IslandWrackTests.cpp`
as the committed source (SHA-256 matched). The focused test covers deterministic
kind/heap selection, descriptions and aging, turning an item and remembering
who found it, JSON round-tripping and rejection of malformed state, reclamation
by the tide, and the shore's 24-item cap. Log:
[`Codex_WrackValidation_MemoryDDC.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/Codex_WrackValidation_MemoryDDC.log).

This test does **not** verify that a real storm mark deposits items on the
loaded Island, that the generated actors sit visibly on its beach, or that a
resident can discover and interact with them in a live world. Those remain the
runtime checks to make before calling Storm Wrack fully validated.

The initial headless automation launch could not access the user's shared
Zen/Derived Data Cache and exited during cache initialization.
`-DDC-ForceMemoryCache` let the focused automation run finish. The first Game
attempt then failed because the shader compiler could not create a transfer
file under `C:\Users\freel\UnrealShaderWorkingDir`. A retry redirected the
shader working and local cache directories into the scratch project; it
remained before the Island world-ready marker for the full 120-second startup
guard and the wrapper terminated only that scratch Game. Logs:
[`Codex_WrackValidation.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/Codex_WrackValidation.log),
[`Spectator.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRuntimeWorld/Spectator.log),
[`Spectator_LocalCache.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/WrackRuntimeWorld/Spectator_LocalCache.log).
The open interactive editor and its unsaved level were left untouched. No
project settings or shared cache were changed.

Next, retry the bounded runtime check when the scratch startup path is
responsive, using a scratch-local shader working directory and cache. Confirm
the `LogIslandWrack` placement count/positions and capture the resulting beach
before testing resident `move_to`/`interact` behavior. Keep agent thinking off,
data isolated, and runtime/request caps explicit.
