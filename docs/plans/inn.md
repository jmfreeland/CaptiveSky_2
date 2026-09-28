# The Inn (plan, 2026-09-27)

A hospitable inn for the Island, in the spirit of the inn in *Quest for Glory I* (Spielburg's Hero's Tale Inn, as best remembered). That inn was the hero's safe home base: a warm, timbered room with a fire and a meal, a kind innkeeper, a bed upstairs where the day ended and the next began, and the place you heard what was going on in the valley. The aim is to carry forward that feeling of *coming in from the weather to somewhere that knows you*, as an original CaptiveSky place rather than a reproduction (see README, Vision and Character inspirations).

## What makes it feel like that inn

- **Arrival from outside.** A lit doorway visible from the path at dusk, and the change in sound and light as you step in out of the wind and rain.
- **A hearth at the centre.** A real fire that the innkeeper tends: lit at dusk, banked at night, and the brightest warm light on the Island after sunset.
- **One common room.** Heavy tables and benches, a counter, a stair to the rooms. It's small enough that anyone inside notices who comes in.
- **A bed that means safety.** Sleeping at the inn is the good ending to a day: dry, warm, and undisturbed.
- **Talk of the valley.** The inn is where news collects. Here, that means what residents and visitors choose to say out loud, not a quest board.
- **A kind keeper.** Warm, a little fussy, remembers you, and never a quest dispenser.

## Where

A level, walkable site on the route from the shore inland, between the shore approach and the Tideglass Pool. It's the first built thing an arrival sees, it can later become where visitors enter the world (and where an "elevator" might stand), and it sits a short walk from the ListeningStones culture area without crowding it. Choose the exact spot with the tools we already have:
- the arranging-ground site finder (level, open, walkable ground);
- a new `02b_Inn` viewpoint in `Config/IslandViewpoints.json`, so the lit doorway is framed from the path at 17:00 and at night;
- the grounding audit once the blockout exists.

## Phases

Each phase is small and shippable, and is checked with tests, captures and a bounded live session.

1. **Blockout** (done 2026-09-27; see README, "The inn blockout"). A headless tool, like `CaptiveSky2.Tools.RepairWindArch`, backs up the map and places a blockout inn from `LevelPrototyping` meshes:
   - a timber-framed common room about 8 × 10 m with a chimney, a door facing the path, windows, a counter, two tables, a stair and one upstairs room;
   - Megascans rock for the hearth and footings.
   
   Every actor is labelled and tagged (`IslandInn`, `InnHearth`, `InnDoor`, `InnBed_1`, ...) so code finds it by tag, not position. Navigation is rebuilt, and the grounding audit and a capture confirm it.
2. **A real shelter** (in progress). Residents perceive being indoors from an actual roof and walls. A read-only collision check requires a tagged inn roof overhead plus tagged inn walls blocking at least six of eight horizontal traces. At a verified indoor listener, camera-centred rain streaks and local roof-splash visuals are hidden, and generated wind/rain ambience is softened to one fifth; Island-wide weather and Tideglass rain responses continue. The `Indoors` 0/1 scalar is published to the environment collection for the player/viewpoint, so future materials and ambience can use the same evidence. Automation exercises the actual resident report, environment value, indoor/outdoor visuals, and ambience scaling without model calls. Rendered play still needs to verify appearance and sound, and no current material yet uses `Indoors`. Do not describe an occupant as completely dry or warm until those conditions are actually modeled and checked.
3. **The hearth** (in progress). In Game/PIE, residents and visitors may kindle the tagged hearth point light and three transient, stylized engine-cone flames; the ensemble gently flickers, may be banked early, and automatically banks after five minutes of play (about three Island hours at the current clock rate). Residents perceive its lit/dark state only when the tagged hearth marker is nearby and visible. Within a smooth 3.5-metre radius while lit, they receive a small simulated proximity-warmth cue; it does not represent body/room temperature, dry shelter, or persistent condition. No material parameter reads hearth state. Next: replace the engine-shape flame placeholders with authored fire art, then consider a real thermal model before publishing measured warmth or shelter effects to the environment system.
4. **Beds that mean safety** (first resident-choice step implemented). Residents perceive an unobstructed tagged `InnBed_n` within 25 m as an optional movement target and must actually arrive before naming it in a sleep action. After the 120-second rest interval completes, the resident's durable memory records the lived rest only when the shared geometric check confirms an `IslandInn` roof overhead and at least six of eight enclosing directions blocked by inn geometry; waking early cancels the pending memory. This does not establish warmth, complete dryness, comfort, or health recovery; sleeping without that evidence remains possible but is not described or remembered as sheltered inn rest. `CaptiveSky2.Agent.IslandInnRest` covers the real perception and sleep gate in an isolated fixture, uses a unique temporary memory directory that it cleans up, and makes no model requests. The tagged bed is now in the loft. Rebuilt saved-map navigation confirms a complete grounded route from the common room to its target (46 m from the ListeningStones), with explicit bidirectional links bridging the two small stair-nav gaps; the target projects within 10 cm of its nav goal. A rendered PIE check should still verify that actual resident movement up the steps and the optional rest choice look natural. The raven remains free to choose a roost.
5. **The guest book** (resident prototype implemented). A transient open book sits on the tagged counter and displays the three latest saved lines. A resident settled on the ground within 2.5 m can use `build` with target `GuestBook` and an `intent` line. It is cleaned to printable single-line text (180 characters), signed with the resident ID, limited to one entry per resident per Island day, and keeps only the 24 most recent lines in `WorldState/<Map>.json`. Nearby residents can read the latest three, and `Island.ForgetGuestBook` reversibly clears them without removing the prop. The isolated automation test covers range, daily limits, cleanup, read-back, cap, persistence, reset and prop refresh without model calls. Still needed: authored book art, rendered player read/write interaction, visitor contributions, and a PIE check of residents choosing to write naturally. It's the inn's memory, and the first shared writing on the Island.
6. **The innkeeper.** A new resident, drafted in `docs/residents/` like Fenrus before it gets a body. Hospitable and a little fussy, attached to the place, with routines that come from the systems above: tending the fire at dusk, noticing arrivals, keeping the book. Not a quest-giver, and not in charge of anyone.
7. **Fenrus under the floorboards (optional).** If Fenrus comes to life, the inn's cellar or the space under the floor is an obvious place for him to choose to live. The innkeeper may or may not know he's there, and certainly doesn't know his name.
8. **Residents shape it.** Bounded, lasting additions like the arrangements: a shelf of found objects, a name carved over the door by vote or by first claim, a room someone keeps returning to.

## Not in scope

No currency, no shop, no health restored by sleeping, no quest board, no scripted innkeeper dialogue. Food and drink stay descriptive at most. The inn should work because it's a good place to be, not because it hands out rewards.

## Open decisions

- **Site:** between the shore and the Tideglass Pool (recommended), or on the ListeningStones terrace from the weekly concept art.
- **Who builds it:** an authored blockout first (recommended, since it's quick to iterate and needs no new building system), or residents raising it piece by piece over Island days.
- **The innkeeper:** a new resident (recommended), or leave the inn keeperless at first and see whether Aster or the rat adopts it.
