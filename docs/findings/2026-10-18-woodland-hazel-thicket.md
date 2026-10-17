# Retraction: hazel-thicket placement candidate — 2026-10-18

The runtime placement briefly introduced in commits `b2f65d6` and `0a867d1`
has been removed. It used the exact local asset
`/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012/SM_WoodlandHazelThicket_01`.
The prior bounded Unreal Game review explicitly rejected this asset for Island
placement: its crown rendered nearly black, sparse, and skeletal at both close
and mid distance. See [the original review](2026-10-12-woodland-hazel-thicket.md)
and its [Game captures](../../Saved/Playtests/Codex_WoodlandHazelThicketGame_20261012/Screenshots/).

The earlier integration did pass UE 5.8.3 header processing and an isolated
scratch-target build, but its focused automation test and its in-world effect
were never verified. Structural import checks and a Blender preview were not
sufficient grounds to override the Game rejection. No Island map or persistent
world state was saved. Do not place this mesh unless its art/material
presentation is substantially reworked and it passes a fresh, bounded Game
review. Prefer adding depth with assets already proven in the intended Game
lighting and composition.
