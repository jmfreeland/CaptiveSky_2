# Morning live session (2026-09-28)

A bounded 25-minute session with spectator mode filming. Aster, the raven and the new innkeeper were all present, and no scripted direction was given. Memory aggregates come from `Scripts/Analyze-AgentMemory.py`, before and after (`Saved/MemoryReports/2026-09-28-before.md` and `-after.md`). Log: `Saved/Logs/LiveMorning_2026-09-28.log`.

## Numbers

- It ran the full 1,500 real seconds and used 73 of 120 model requests, making 70 decisions: Aster 25, the raven 26, the innkeeper 19.
- By action: waiting 28, speaking 19, moving 17, inspecting 4, wandering 2. Two days earlier, 30 of 39 decisions were speech. The social pacing change (`148f0ea`) is doing its job.

## Memory

- New reflections: Aster 2, the raven 0, the innkeeper 1. Before the fixes in `d365531`, near-duplicate reflections accumulated by the hundred.
- What retrieval puts in Aster's prompt, for the analyzer's standard situation, is now 10 reflections, 5 conversation lines and 2 observations. On 27 September it was 19 of 19 small-talk lines.
- Historical duplicate rates are unchanged (86% and 75%), as expected: nothing rewrites old memories.

## What residents did

- **The innkeeper** kindled the dark hearth first ("a small comfort worth offering to the room"), later offered a greeting, and waited out the rain by the fire.
- **The raven** perched on its roosts, rang the ListeningStones twice (now tuned to the wind) and quietly watched a firefly.
- **Aster and the raven** spent the morning noticing "a thin mark" and "faint points" in the brightening sky, and were careful to say they couldn't explain them.
- **Sleep:** Aster and the innkeeper slept at night. Neither verified the sheltered inn bed.
- **Not reached:** nobody reached the cairn, the pale-stone trail, the arranging grounds or the guest book.

## Problems found

1. **Mangled approach targets (fixed in `570331b`).** Perception offers `ApproachAgent_Agent_Innkeeper_01`, and the model tidied it to `ApproachAgent_Innkeeper_01`. Both approach attempts failed as "target not found". Residents now also answer to the shorter form.
2. **The innkeeper couldn't leave the inn (open, noted for Codex).** Three attempts, including a `wander` to a navigation-reachable point, ended "Movement did not complete". Its start beside the hearth may be physically boxed in by the hearth, mantel or back table.
3. **The raven's flights were blocked** four times ("Flight was blocked by geometry"). The raven's straight-line flight has no obstacle avoidance. It isn't new, but with the inn standing on the route it's more common.
4. **The interior inn viewpoint is blocked.** Since the tables moved deeper into the common room, the `02c_InnCommonRoom` camera sits against the front table. `Config/IslandViewpoints.json` needs a new camera position.
5. **Startup bursts.** Short capped runs on 27 September used their whole request budget within seconds. The continuous mode in `570331b` starts its allowance half full and spaces each resident's requests.
