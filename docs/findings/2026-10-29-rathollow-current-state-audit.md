# Rat hollow current-state audit — 2026-10-29

## Verified

The main UE 5.8 editor passed `CaptiveSky2.Agent.WildlifeHomeRecord` with result `Success`. The regression covers canonical home serialization, a transient RatHollow actor, inspection as a read-only interaction, hidden provisional caches, settlement after three distinct rest days, disturbance behavior, and persistence round-tripping. The local hollow mesh and Hazel-nut branch referenced by the materializer are present and load in the test fixture.

## Not yet true in the Island

- The current project-root `WorldState/Island.json` (last saved 2026-10-28 23:44 UTC) has an empty `wildlife_homes` array.
- `Agents/Agent_Rat_01/places.json` is also empty.
- The automation uses a temporary fixture record; it does not prove the Rat spawns at a canonical Island home or that the home is visually concealed and navigable in the saved map.
- The earlier bounded site review rejected the first human-height location `(-104500, 105000)` as an exposed, repetitive slope. Two nearby candidates, `(-105200, 104200, 2334)` and `(-103800, 104200, 2375)` cm, passed the audit's ground/understory/nav/approach checks but still need human-height and Rat-eye real-RHI review. See [the site expansion review](2026-10-12-rat-hollow-site-expansion.md).
- After the editor automation, a whole-device `nvidia-smi` sample showed 9,360 / 12,282 MiB in use (about 76%) at 36% utilization. This is not an Unreal-only or peak allocation, but it does not establish safe headroom for a PIE visual review.

## Next step

Keep the home record empty until one of those candidates has passed paired visual review and a final placement/navigation check. When GPU headroom is safe, use a disposable preview for both camera heights; then create the canonical home at the selected, verified entrance and confirm the Rat's actual Game spawn. The current editor automation validates the underlying resident-agency system, not that final in-world result.
