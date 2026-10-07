# Tideglass dragonfly wing silhouette (2026-10-07)

Replaced the four primitive ellipsoids on each of the three daytime Tideglass
dragonflies with one small swept, tapered, double-sided procedural surface per
wing. The surfaces keep the stable green/blue/copper morph palette, are
collisionless, cast no shadows, and cannot affect navigation. Existing body
parts, population cap, habitat, time-of-day lifecycle, wind/rain behavior,
resident interaction, and low-raven-flyby response are unchanged. The mesh is
created outside Tick; in flight only the four component rotations animate.
There are no imported assets, new model requests, or persistent state changes.

The UE 5.8.3 `CaptiveSky_2Editor` target built successfully. The focused
`CaptiveSky2.Agent.NightEcology` automation passed with agent thinking disabled,
a 120-second realtime cap, and zero model requests. It checks one finite
geometry section per wing, tapered-tip vertex and bounds finiteness, and
collision/shadow/navigation exclusions, along with the existing ecology and
raven-response behavior. Log:
[`Codex_DragonflyWings2_20261007.log`](../../Saved/Logs/Codex_DragonflyWings2_20261007.log).

The matched real-RHI daytime Tideglass preview spawned exactly three transient
dragonflies, passed `CaptiveSky2.Visual.Viewpoints`, and reported no renderer
NaN bounds after the tip clamp. It ran with `-ViewpointNoWorldState`; preview
actors are removed after the screenshot. Log:
[`Codex_DragonflyWingView2_20261007.log`](../../Saved/Logs/Codex_DragonflyWingView2_20261007.log).
The wide shore capture is
[`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-07_001527_h11.0/02_Tideglass.png).
It verifies that the Island can render the preview without errors, but the
camera is too wide to judge the dragonfly's wing silhouette confidently.
No in-game frame-time or close-range visual comparison was measured.

## Next check

When the interactive editor is available, inspect a normal resident-height
Tideglass view and confirm the new wings remain legible while fluttering. If
they read as opaque slivers or add measurable cost, revise or remove this
procedural treatment before adding detail or a new asset dependency.

## Hindwing readability follow-up

The earlier close-view review suggested that the four identical narrow fans read
as wing streaks. The rear pair now has a 39 cm span and 6.8 cm maximum half-width;
the front pair remains 33 cm by 4.4 cm. This introduces the broader hindwing
outline characteristic of dragonflies without adding assets, lights, shadows,
collision, ticking, or persistent world-state changes.

The ecology regression now checks that the generated hindwings are at least 25%
broader and 10% longer than the forewings. The UE 5.8.3 editor target build
succeeded (7 actions), compiling both the actor and test source. Three
`UnrealEditor-Cmd` automation attempts returned only platform-SDK validation
output, creating no requested test log; therefore the new assertion has not run.
The interactive editor/MCP was unavailable, so the changed outline still needs
a passing `NightEcology` run and a resident-height capture before this art pass
can be called done.
