# Tideglass wet-edge ground-cover composition

## Change

The deterministic Tideglass scatter now fades a wet-edge influence out from the verified flattened pool surface. Within the band, up to 38% of existing seven-metre botanical patches are remapped to the project's existing broadleaf ground-cover variants. The normal eleven-species pool remains in use, and no scatter candidates or foliage instances are added. A second 125 cm sample margin accounts for broadleaf mesh-pivot offsets near the water.

The moisture influence uses a smooth fade from 100 cm beyond the computed pool clearance radius to 1,800 cm beyond it. The closest other landmark retains its own ground-cover composition; ties also favour the other habitat. Without a detected pool surface, there is no wet-edge bias. This is a static place-based composition rule, not a claim that soil moisture or plant growth is simulated.

## Evidence

The isolated UE 5.8.3 target compiled successfully. `CaptiveSky2.Agent.GroundCover` passed after checking wet-edge fade distances, other-landmark ownership, deterministic patch selection, the existing species range, pool clearance, fixed population accounting, and the existing placement/wind/navigation invariants.

A thinking-disabled real-RHI spectator render at 17:00 captured the updated Tideglass view. It shows the broadleaf-rich wet margin and the existing woodland edge; the selected image is [Tideglass-Wet-Edge-at-Dusk](../../Saved/Highlights/2026-10-04/1942/Tideglass-Wet-Edge-at-Dusk.png). The render reused the project map and did not make LLM requests. No matched before/after performance comparison was performed, so this capture is visual inspection only.

## Next

Use the same camera for a matched before/after ground-cover profile and check that the broadened pool margin still reads clearly at walking distance. Continue the ecological bands along the Tideglass-to-Wind-Arch sightline only after that controlled comparison; do not compensate by increasing the global instance budget.
