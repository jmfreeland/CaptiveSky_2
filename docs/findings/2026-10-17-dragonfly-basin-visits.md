# Dragonfly rain-basin visits

## Change

Tideglass dragonflies now briefly investigate the existing persistent rain basin when it contains enough water to float a leaf. A visit is limited to 6.5 seconds, has a 22-second cooldown, and stations each color morph just outside the rim and above the water level. The scan is local (12 m), paced at 1.6 seconds, and suppressed during heavy rain, startle, ripple interest, or a flower visit. Heavy rain, a nearby ripple, or a quiet visitor can interrupt a visit. It does not land, move leaves, mutate basin state, call a model, or write memory.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target built successfully from the isolated `Codex_VisitorResonance_20261017` scratch project on 2026-10-17 (11 actions, including `IslandTideglassDragonfly.cpp` and `IslandEcologyTests.cpp`).
- The focused `CaptiveSky2.Agent.NightEcology` automation includes cases for the dry threshold, hover geometry, approach, rain interruption, and no immediate retry. It was compiled but not run: the current full-content scratch automation launch has the previously documented Core null-access startup failure (`COORDINATION.md`, 2026-10-17). No in-editor visual/play check was possible because the current desktop connection exposes no app windows.
- The live distance between Tideglass and the basin remains unverified. The behavior only activates when a dragonfly's actual patrol position comes within 12 m; it does not pull a resident across the island.

## Next check

When the editor is available and the Island is running, wait for the basin to reach the floating-leaf water threshold, then observe whether any dragonfly patrol passes within 12 m. If not, choose a resident whose established habitat is already near ListeningStones rather than expanding flight range blindly. Confirm the hover clears the basin rim and reads well from visitor height.
