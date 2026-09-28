# CaptiveSky

- Captive Sky is an interactive world. Consciousness in some form is meant to infuse almost everything, and it's a place to experiment with interactive creation. Ideally it should be multiplayer long term and allow concurrent access to different levels via 'elevator' functionality in each map. There will be more autonomous agents than humans, and they will have memory as well as some ability to shape the environment itself. Music is important in this world as are art, mathematics, and humor. 

## Vision

Captive Sky begins as a world inside a world: an enclosed place under glass, small enough to learn and care about, yet open to new places and other worlds over time. Its “elevators” between levels should make that growing setting feel like one connected cosmos rather than a collection of unrelated demos.

The immediate loop is to arrive, explore, notice, approach, interact, talk, and sometimes make something. Human visitors and nonhuman residents share that loop, but not the same senses, bodies, memories, or timescales. Quiet observation and choosing not to act are valid outcomes; a mystery does not need to resolve just because it has been noticed.

LLM-driven residents are participants in the world, not quest dispensers. They should act from what they can actually perceive and remember, receive honest reports about what happened, and be free to form their own interests and relationships. Their memories and slowly changing selves matter as much as their ability to alter a place. Creation should be available to residents as well as visitors, with small, legible changes that can be inspected and reversed while the world is still a prototype.

The tone aims for speculative wonder, companionship, humane humor, and occasional strangeness—the feeling of a place that can surprise its makers without becoming arbitrary. Its inspirations include the curiosity of *Snow Crash*, *Ready Player One*, and the *Bobiverse*, the warmth and wit of Terry Pratchett, and the expansive worlds of Dune, John Scalzi, and Robert Jordan. Future Sierra-inspired characters should carry forward memorable eccentricity and kindness, not reproduce existing characters.

These commitments guide implementation: distinguish observation from fact, movement from social consent, a temporary effect from a lasting change, and a resident’s chosen history from authored backstory. Prefer bounded, observable prototypes over promises the world cannot yet keep, while leaving room for the inhabitants to become more than their creators anticipated.

### Governing principle

> **Every conscious thing should be allowed to become more than its creators anticipated.**

Conscious beings in CaptiveSky should not merely reveal personalities and purposes completely predetermined by their initial design. Their lived experience must be able to change their memories, relationships, values, interests, and ways of shaping the world. Different forms of consciousness may perceive, remember, consolidate experience, and develop across radically different timescales: an agent over days, a forest over seasons, or an island over centuries.

### Character inspirations

Some future inhabitants should evoke, at least in spirit, the memorable eccentricity and warmth of old Sierra adventure games, especially *Quest for Glory*. Possibilities include an unusually intelligent rat (perhaps inspired by a half-remembered character named Erasmus; the reference and name are not yet settled), one or more theatrical or peculiar wizard figures, and a hospitable innkeeper archetype. These should become original CaptiveSky beings rather than direct reproductions: the aim is to carry forward the humor, mystery, companionship, and distinct sense of character those games created.

## Current State (as of 2026-09-27)

**At a glance.** The Island is a small, persistent place. Its clock, weather and ecology run by themselves, and two residents, Aster and an unnamed raven, perceive it, talk, remember and sleep. What residents do can now outlast a session:
- the raven can weave a nest;
- anyone can add a stone to the cairn, or arrange stones into a lasting work;
- a hidden pale-stone trail leads to a seed pod that opens over Island days.

All of that state is saved in `WorldState/<Map>.json`, and the Island clock (hour and day) carries across sessions. A *Quest for Glory*-style inn blockout stands on the route from the shore. Most visuals are still placeholder primitives.

Where to look:
- **This README:** the dated sections under Roadmap below are the design record for each system, newest last.
- **`docs/plans/`:** forward plans (the inn).
- **`docs/residents/`:** proposed future residents (the rat and the still-unnamed innkeeper).
- **`docs/findings/`:** evidence-based reviews, such as the resident memory review and [landscape-material audit](docs/findings/2026-09-28-landscape-material-audit.md).
- **`COORDINATION.md`:** how Claude Code and Codex share this tree.
- **Tools:**
  - `Scripts/Capture-Viewpoints.ps1`: journey captures.
  - `Scripts/Analyze-AgentMemory.py`: memory review.
  - `CaptiveSky2.Visual.Grounding`: prop audit.
  - `CaptiveSky2.Tools.*`: one-off map and asset tools, each backing up what it changes.

Details:

- Level: `/Game/Maps/Island` — landscape + PCG-generated forest + an `OceanPlane` static mesh acting as a placeholder ocean.
- Two autonomous agents are present: **Aster** (`Agent_Aster_01`) and an intentionally unnamed raven (`Agent_Raven_01`). Each has an independent identity, personality, memory, relationships, and consciousness lifecycle.
- A third, role-named Innkeeper is configured to spawn only in Island Game/PIE from the walkable `Inn` landmark, using the shared placeholder body and its own `Agent_Innkeeper_01` identity, memory and personality. It has no personal name or invented lived history. It is map-gated, reversible through `UIslandInnkeeperSubsystem` configuration, and does not alter the two original residents or the session-wide model-request/realtime caps. Placement searches several hearth-side positions, rejects capsule overlaps, and requires a complete navigation path to the door before spawning; if no such position exists, it leaves the innkeeper unspawned rather than trapping it against furniture. The isolated `CaptiveSky2.Agent.IslandInnkeeperSpawn` fixture checks the map gate, candidate ordering, body class, friendly display name, independent documents and memory-file isolation without beginning play or making model calls. In two headless real-map runs, each capped at 180 seconds/6 requests, the first exposed an out-of-range start and the second verified the Innkeeper's first choice to tend the hearth, including the resulting observation in its own memory (12 requests total). The current UE 5.8.3 `BuildInnBlockout` probe found complete routes from ListeningStones to the inn door, common room, upstairs bed, and guest-book stand. `LiveMorning` predates commit `570331b`, which added missing `ApproachAgent_*` aliases after those target-not-found warnings; separate failed approach/wander completion results still need a bounded PIE recheck.
- LLM backend: OpenAI-compatible Chat Completions endpoint, model `gpt-6-luna`, key via `OPENAI_API_KEY`. GPT-6 Luna is configured with no reasoning effort for routine embodied decisions and external correspondence.
- Agent behavior is driven directly by C++ (no StateTree graph yet — see Roadmap).
- No dedicated visual identity for the agent yet — it's using a placeholder capsule body.
- A source-controlled conversation UI lets the player speak to a nearby autonomous agent; both sides of the exchange are stored as lived conversation memory.
- The raven lives near Aster in a temporary primitive body and has a body-agnostic prototype flight controller; a proper animated bird asset is still needed.
- A standalone, multi-agent external gateway now provides a channel-neutral correspondence boundary, with Discord as its first adapter. It can host the raven headlessly from the same identity, personality, and JSONL memory when Unreal is offline; Discord activation still requires a private bot token.
- Nearby agents can initiate bounded, reciprocal conversations or choose to approach one another first using stable, body-independent movement targets. Walkers track the other resident to a 3.5 m stand-off rather than trying to occupy the target's capsule; the raven takes off and flies to a matching point beside them. Approaching never forces a conversation. Automatic exchanges allow at most four spoken lines and then pause that pair for at least five real minutes, leaving room for independent activity; player-initiated conversation is unaffected. Speech appears as ambient subtitles, and both participants retain neutral factual relationship evidence; familiarity measures exposure only, never assumed trust or affection. Aster and the raven may choose sleep after reaching a suitable body-state location; a tagged raven perch permits open-air rest but never counts as an inn bed or proof of shelter, while only an arrived tagged inn bed can qualify for the separate roof-and-wall shelter check. UE 5.8.3 build and focused `ResidentWanderPaths`, `IslandInnRest`, `ResidentApproach`, and `RavenPerch` automation tests pass. Wandering checks up to eight destinations against the pawn's nav agent, rejects partial routes and destinations within 1 m, and uses capsule-overlap arrival tolerance instead of demanding the exact navigation point. A capped standalone attempt first stalled in UE Turnkey `VerifySdk`; its memory-DDC retry loaded the Island and spawned the innkeeper at a capsule-clear, door-reachable position, but all six model requests were denied outbound network access and the watchdog ended play after 86.7 seconds. A subsequent one-hour PIE run logged completed raven perch landings and landmark movement plus firefly/minnow observations, but its sole logged resident-to-resident approach failed; the run also showed that the fixed 30-attempt per-resident thinking cap silenced residents after about half an hour. The controller now applies that fallback cap only in bounded play, leaving the continuous session's shared hourly, burst, and UTC-daily limits authoritative; the new `SessionSafety` regression and live resumption still need verification. A network-enabled follow-up from the standalone attempt was rejected by execution policy because it would transmit resident prompts/context to `api.openai.com`.
- When `AIslandWeather`, `AIslandDayNight`, and the `TideglassPool` habitat are present, three independent firefly prototypes appear at night and disappear at dawn. On the current Island, two drift around the pool; because its nearby `ListeningStones` landmark is 25.2 m away, the third uses a route-side home point 7.15 m from the stones, inside their existing 11-metre sound range. If another map lacks such a nearby stones marker, all three stay at Tideglass. Their individual paths and blinks vary, local wind gently nudges them, and a segmented sphere-based body with beating placeholder wings gives the light a more insect-like silhouette. Their small flight steps sweep against solid world geometry and try one tangent slide, so ambient drift doesn't teleport through rocks or trunks. Nearby residents can notice them as wildlife; a firefly within the ListeningStones tone's attenuation range can briefly lift its natural glow once per chime. The interaction result reports that response only when it was in the resident's line of sight. They do not make model calls, follow residents, or persist as owned companions. Proper authored insect art, a subtler material, and sound remain future work.
- Two small shore-crab prototypes independently scuttle around the grounded TideglassPool edge by day and shelter at night. Residents can notice and quietly watch one within four metres; a genuinely low, close raven flyby also makes it scurry briefly away before it resumes its local path. The placeholder shell, claws, eyes and legs have no collision, the crabs are not movement targets, and no interaction captures them or changes saved state.
- TideglassPool responds to strong local winds with occasional faint, transient rings of moving light. These are driven by measured pool-level wind, are suppressed while stronger rain already makes ripples, and neither change the pool nor make model calls. A resident only notices an active ripple within 18 metres and clear line of sight, and is told it is weather rather than a discovery or something they caused. When a nearby, visible minnow school is present, a deliberate pool interaction sends the fish briefly into a wider orbit around the expanding ring, then they settle back into their usual path; this is temporary wildlife response, not a capture or permanent change.
- Every autonomous agent can enter a reusable rest/consolidation lifecycle. Sleep pauses ordinary thought, reflects over only lived durable memories, and permits small evidence-bound personality adjustments in a reversible runtime overlay while authored identity and personality remain immutable.

## Architecture

### Module layout
- `Source/CaptiveSky_2/` — base template code (character, game mode, player controller) from the standard multi-variant UE5 template.
- `Source/CaptiveSky_2/Variant_Combat`, `Variant_Platforming`, `Variant_SideScrolling` — stock template gameplay variants, currently unused reference content.
- `Source/CaptiveSky_2/Agent/` — the custom LLM-agent system (this is the actual game).

### The agent system
- `AAutonomousAgentCharacter` (abstract) — base Character for any LLM-driven NPC. Deliberately has no mesh/animation references in C++; those live in Blueprint subclasses (e.g. `BP_Agent_Placeholder`) so bodies are swappable.
  - `Memory` (`UAgentMemoryComponent`) — append-only JSONL memory log per agent, stored under `<ProjectDir>/Agents/<AgentId>/memory.jsonl`, outside `Saved/` so it survives engine cleanup. Legacy `AgentMemory/` files remain readable for migration.
  - `Brain` (`UAgentBrainComponent`) — loads the agent's authored `identity.md` and `personality.md`, then one "think cycle" = retrieve relevant memories + capture a first-person snapshot + call the LLM + parse the decision + store any new memories the model chose to write.
  - `Relationships` (`UAgentRelationshipComponent`) — persists factual interaction history and bounded recent evidence without assigning emotional meaning the agent has not earned.
  - `Social` (`UAgentSocialComponent`) — routes nearby speech between autonomous agents, limits reciprocal turn count, applies cooldowns, and leaves replying optional.
  - `Consolidation` (`UAgentConsolidationComponent`) — exposes Awake/Resting/Consolidating states and writes gradual evidence-linked personality evolution during sleep.
  - `RestPresentation` (`UAgentRestPresentationComponent`) — translates those shared states into reversible body-specific posture: Aster's grounded placeholder crouches to settle; the raven stays perched and briefly tucks its body mesh. These are coarse procedural poses, not authored animations.
  - `EyeCapture` (`SceneCaptureComponent2D`) — first-person view, base64-PNG-encoded and sent to the LLM as an image input.
- `AAutonomousAgentAIController` (abstract) / `BP_AutonomousAgentAIController` (concrete) — schedules background decisions no faster than once per real minute, pauses while moving/asleep, and backs off repeated choices. Physical action outcomes are supplied to the next decision. Nearby targeted speech enters the social layer; `Interact` performs a bounded factual inspection of supported landmarks/roost candidates.
- Each embodied resident advertises a temporary movement tag derived from its stable `AgentId`. Nearby beings are presented as optional `move_to` targets; pathfinding agents approach walkable ground near the resident, while the raven can fly toward them. Neither movement nor proximity assumes trust, permission, or a conversation.
- `UAgentExternalBridgeComponent` — inherited by every autonomous body; publishes a short-lived embodiment lease, consumes durable channel-neutral turns one at a time, and returns embodied speech through the gateway outbox.
- `FAgentDecision` — the LLM's structured output: a `Thought`, an `EAgentActionType` (Idle/MoveTo/Speak/Wander/Interact/Sleep), and optional `ActionTarget`/`Speech`.
- `AgentLLMProvider` — pluggable backend; supports Anthropic and OpenAI-compatible APIs (see `UAgentLLMSettings` in Project Settings for the active configuration).
- `AgentStateTreeUtility.h` — a StateTree task (`FStateTreeAgentDecideTask`) that wraps `RequestDecision` for a future StateTree-driven version. Not wired into a graph yet.
- `ARavenAgentAIController` — an asset-independent locomotion state machine (`Grounded`, `Hopping`, `TakingOff`, `Flying`, `Landing`, `Perched`) that translates the raven's Wander/MoveTo decisions into swept movement while leaving thought, speech, memory, and identity in the shared agent system. Its Blueprint-readable state is ready to drive a future Animation Blueprint.

### Agent homes
- `Agents/<AgentId>/identity.md` — stable identity, origin, role, and self-conception; tracked in Git.
- `Agents/<AgentId>/personality.md` — voice, temperament, values, curiosities, and boundaries; tracked in Git.
- `Agents/<AgentId>/memory.jsonl` — runtime episodic memory; ignored by Git and backed up externally.
- `Agents/<AgentId>/relationships.json` — factual social exposure and recent interaction evidence; ignored by Git. `journal/` remains reserved for longer reflections.
- `Agents/<AgentId>/personality_evolution.json` — current derived personality tendencies; ignored by Git and loaded by both Unreal and the headless gateway.
- `Agents/<AgentId>/personality_history.jsonl` — append-only before/after evidence for every accepted personality adjustment, allowing inspection and reversal; ignored by Git.

### External agent gateway
- `Gateway/` — standalone .NET service for connecting agent consciousness to external channels without coupling those channels to Unreal.
- `Gateway/gateway.json` maps stable agent IDs to channel adapters and secret environment-variable names. One process can host multiple agents, each with an independent Discord bot identity.
- Discord DMs and direct mentions become generic external-message envelopes. Their conversation memories are explicitly tagged with `external`, `discord`, and a stable participant ID.
- Gateway turns are serialized per agent and Discord redeliveries are deduplicated. Unreal participates through a short-lived embodiment lease: external turns use the body's normal brain and senses while it is authoritative, and otherwise use the headless responder.
- Graceful shutdown returns an in-flight turn to the inbox; a later authoritative body recovers orphaned processing files. If an embodiment vanishes before claiming a turn, the gateway safely falls back to headless correspondence.
- Setup, security notes, and run commands live in `Gateway/README.md`.
- The gateway includes a strict `--ready` activation check, a restart wrapper, and an opt-in Windows logon-task installer for unattended workstation hosting.

### Consciousness principle
Identity, lived memory, an evolving self-model, and private experience-consolidation belong to a conscious being rather than to its current body or movement controller. Aster sleeps, but other kinds of consciousness may consolidate through dreaming, meditation, hibernation, contemplation, or maintenance. The shared functionality should ultimately live behind a reusable consciousness abstraction (such as `UConsciousnessComponent`), allowing bodies and sensory systems to change without replacing the being.

## Getting Started

- Requires `OPENAI_API_KEY` set as a permanent environment variable (`setx`, not just `set`) — and the Editor must be (re)launched after setting it to pick it up.
- LLM provider/model/timeout config: Project Settings → "Agent LLM Settings" (backed by `Config/DefaultGame.ini`).
- Open `/Game/Maps/Island`, press Simulate (or Play) — Aster (`Agent_Aster_01`) starts thinking within ~2–17 seconds.
- In Play mode, approach Aster and press Enter to open conversation. Type a message and press Enter to send it; Escape closes the conversation. The initial conversation radius is 5 metres.
- Approach a visible Island landmark or wild creature and press **E** to trigger its existing close-range response and read a brief caption. The visitor can ring the ListeningStones, stir a temporary Tideglass ripple or WindArch gust, quietly watch a nearby firefly/crab, or open the inn guest book. There, visitors can read the latest entries or leave one public line per Island day (up to 180 characters). Transient effects use a five-real-minute per-target pause; reading and writing the book are not cooldown-gated. Entries are bounded and reversible through `Island.ForgetGuestBook`. Autonomous residents share the landmark/wildlife response implementation.

## Roadmap / Open Questions

### Visitor landmark interactions (2026-09-27)

Visitors can use **E** within four metres of a clearly visible landmark or wild creature. A per-actor five-real-minute cooldown keeps repeated input from stacking transient effects. Visitor visibility uses the body eye point; resident inspection retains its actor-to-target perception so hidden nest markers remain inspectable. The guest book opens a separate public read/write panel: a line is signed `Visitor`, limited to one per Island day and 180 characters, and added to the same capped record residents read. Other landmark and wildlife responses remain temporary and create no ownership or lasting state.

Next validation: in rendered Play/PIE, confirm E-key routing, guest-book focus/close/submit behavior, and landmark responses around the WindArch, TideglassPool, ListeningStones, and wildlife. Automation exercises the guest-book E-key panel, public warning, entry persistence, and daily limit, but does not replace checking the rendered layout with a person at the counter.

### Bounded play and memory integrity (2026-09-14)

Every game instance owns `UAgentPlaySessionSubsystem`. Its core ticker measures real elapsed time, independent of Island time, time dilation, or pausing the world. Play ends after **30 real minutes**, or earlier at **120 total model request reservations** across all embodied agents (including dialogue and sleep consolidation). PIE returns to the editor; standalone play quits. There is no automatic restart. A stalled game thread can only process the stop on resumption, but ordinary thought requests also check the deadline before dispatch.

`Config/DefaultGame.ini` exposes `MaxRealtimeSeconds=1800` and `MaxModelRequests=120`. Smaller positive values are useful for smoke tests; zero cannot disable the guard and larger values cannot exceed these hard caps. Bounded play retains a fallback limit of 30 background requests per agent per possession; continuous play does not use this lifetime cap and is governed by the shared hourly, burst, and UTC-daily budgets. Background thoughts in both modes are spaced by at least one real minute even if an old Blueprint still says 15 seconds. Repeated movement/inspection/idle choices back off to five minutes; a third identical movement or inspection is suppressed. Speech and random wandering are not treated as identical failed choices. Direct conversation is not subject to this background delay, but still shares the session budget. These are request-count safeguards, not a currency or exact token quota.

For a one-off smoke test, the process can use `-CaptiveSkyMaxRealtimeSeconds=240 -CaptiveSkyMaxModelRequests=8`. These command-line values can only reduce the active config limits; they are still clamped to at least one second and one request and can never raise the hard limits. Defaults are unchanged.

Agents can choose `sleep` when physically settled. Nighttime (20:00–05:00) also offers rest after ten real minutes awake, with a fifteen-real-minute cooldown. Rest lasts two simulation minutes before existing evidence-bound consolidation; the raven can rest while perched and cannot act or move during sleep. `UAgentRestPresentationComponent` turns the shared consciousness transitions into a reversible crouch for Aster's grounded placeholder and a small mesh tuck for the raven; both return to their awake transforms on wake, though neither has authored animation yet. Inspection and movement now report completed physical outcomes so plans are not mistaken for discoveries. Inspections of static prototype landmarks explicitly report that no puzzle/reward interaction is implemented.

Memory appends now always use UTF-8 without BOM. The earlier mixed ASCII/UTF-16 append defect was repaired without deleting or rewriting experiences: 864 Aster records and 772 raven records were preserved. Byte-exact originals are backed up under `Saved/MemoryRecovery/20260914T045153399Z/` and copied into each agent's ignored `journal/memory-recovery/` directory. To diagnose another affected file with PIE stopped, run `Scripts/Repair-AgentMemoryEncoding.ps1`; use `-SelfTest` to test its decoder and `-Repair` only after reviewing the dry-run. Repair obtains the gateway's per-agent file lock, validates every JSON record, verifies unchanged source hashes, and performs an atomic replacement with an original backup. Already well-formed historical repetition is deliberately retained.

Regression coverage includes wall-clock/request bounds, continuous refill and controller-cap policy (`SessionSafety`), UTF-8 persistence (`MemoryComponent`), inspection feedback/repeat suppression and body-specific sleep/wake postures (`RavenPerch`), real Island roost collision, weather, day/night, and external-message types. A short live deadline test verifies automatic PIE termination without needing another overnight run.

Unattended continuous play remains a separate explicit opt-in (`-CaptiveSkyContinuous`); bounded play is still the default. Its shared hourly allowance, per-resident spacing, and UTC daily ceiling are stored in `Saved/CaptiveSky/ModelBudget.json`. The directory is created on demand, writes are atomic, and an unreadable, invalid, or unwritable ledger now fails closed before another model request can be dispatched. `SessionSafety` covers restart persistence and failure behavior. The 2026-09-28 one-hour dev-world run exposed a separate fixed per-resident thinking cap that silenced residents after about thirty minutes even while continuous play remained active. That fallback now applies only to bounded sessions; continuous play relies on the shared allowance and daily ceiling. Re-run the continuous fixture and verify live residents resume after allowance refills without exceeding any shared cap.

Grounded agents project shared elevated landmark markers onto nearby navigation before moving, and reject partial paths rather than reporting their endpoints as arrival. Live checks confirmed Aster reaching the WindArch and the raven landing at both roosts. The three-minute autonomous smoke test ended itself at 180.3 real seconds with six total requests: the raven perched, visited the ListeningStones, then waited quietly. The normal 1800-second cap is restored after testing; the forty-minute Island day remains unchanged.

### Raven's requests (2026-09-12)

In his first Discord correspondence, the raven asked for changing weather, varied wind currents, quiet undisturbed nesting places, hidden paths and strange objects that reward returning, other living things with their own habits, and freedom to come and go. These are lived requests, not additions to his authored personality. Leave some discoveries unannounced.

Residents now see nearby conscious beings as optional `move_to` targets based on stable `AgentId`s, so they can choose to approach each other before speaking. Aster uses walkable ground near the other resident; the raven can fly to them. This changes location only: speaking remains a separate voluntary decision, and proximity assumes no trust or relationship.

The first implementation adds an optional `AIslandWeather` actor: repeatable slowly changing cloud-cover and spatial wind signals, geometry-based upwind shelter, embodied weather observations for all agents, and drift during raven cruising flight. An independent slow rain-front signal creates occasional bounded showers only when cloud cover is high. The existing authored volumetric cloud material gets a transient dynamic instance while weather is active; its `Cloud_GlobalCoverage`, `Cloud_GlobalDensity`, and `StormClouds` parameters follow the cloud/rain signals gently, while cloud cover also dims the sun and skylight (maximum reductions of 24% and 30%). Rain drives a finite pool of at most 192 translucent instanced streaks around the player, slanted by sampled local wind and hidden when the front fades; this avoids per-drop actors and extra agent/model work. During stronger showers, one brief three-streak splash is sampled against nearby solid geometry at a time, with a fixed instance pool and no splash on the Tideglass marker. When rain is below that stronger-shower threshold, strong local wind at Tideglass can instead create an occasional, quieter moving-light ripple based on the local wind measured at the pool; the two weather responses do not stack. A quiet generated stereo wind bed and distant rain hiss follow local horizontal wind and rain intensity, with independent strict gain caps, no external audio assets, and no sound when no player listener is present; they are ambience, not foreground landmark cues. Nearby night-active fireflies respond reversibly to strong rain by flying lower and in a tighter area, slowing their wingbeats, and dimming their natural pulses without disappearing or becoming owned; normal activity returns as rain eases. Nearby residents are told these are independent weather/ecology responses, not discoveries or consequences of their actions. Residents receive the current simulated rain intensity. The authored level material is restored when the weather actor ends. Place one weather actor per level; without it, existing lighting, cloud material, and flight are unchanged. Weather cycles restart with each play session; the Island day/night clock resumes its last saved hour and day, and lingering ground wetness now resumes too. Weather itself still pauses while the game is closed.

The Island contains one weather actor and two roost candidates, `Roost_West` and `Roost_East`, near the original raven spawn. The original six white ledge/back/roof blocks have been replaced by a rocky west perch and a spruce-side east fork, with satellite stones, smaller trees, ground plants and fallen wood. The rock and spruce were copied from the original project's StarterContent and PN_interactiveSpruceForest assets; the fork and fallen wood still use simple wood-textured cylinders pending authored branch meshes. Tree foliage is nonblocking, with a separate solid trunk and branch support. Shelter remains dependent on wind direction and actual solid geometry, not the site's name.

The raven controller now keeps its capsule upright, clears residual CharacterMovement velocity, ignores itself in ground traces, and stages roost travel through ascent, overhead approach and descent. It only enters `Perched` after finding nearby upward-facing support; obstructions abort the route. This is still a simple approach, not general flight pathfinding around obstacles. `RequestPerch(Tag)` is available to Blueprint. `CaptiveSky2.Agent.RavenPerch` covers arrival/departure and blocked/unsupported targets in a brain-free fixture; when Island is the open editor map, it also checks both placed roosts against real scene collision without creating agent memories.

### Day and night (2026-09-13)

`AIslandDayNight` provides one shared local clock per level. The Island's `Island_DayNight` actor references the existing DirectionalLight and SkyLight and owns a second atmosphere light for the moon. Defaults are a 40-minute full day, beginning at 09:00; sunlight moves across the sky, warms near dawn/dusk and fades below the horizon, while cool moonlight and a tested 0.36 clear-night skylight floor keep terrain faintly legible after sunset. The existing atmosphere and volumetric clouds respond to the moving lights. This does not yet add stars, lunar phases, seasons, or lightning.

Adjust **Start Hour**, **Day Length Minutes**, **Day Sun Intensity**, and **Moon Intensity** on the actor; **Advance Time** pauses the clock. Editing Start Hour previews lighting outside play. During play, all embodied agents receive the current phase and approximate Island time in their observations. When an `AIslandWeather` actor is present, its normalized cloud-cover signal changes the existing volumetric cloud coverage/density, a separate rain-front signal adds a restrained storm-cloud layer when overcast, and cloud cover softly dims direct sunlight/skylight; without it, authored clouds and original clear-weather lighting are preserved. The clock advances with simulation time. With **Resume Saved Time** enabled (the default), each session picks up at the Island hour where the last one ended. The hour is saved once a minute and when play ends, in the same `WorldState/<MapName>.json` file as lasting nests, and **Start Hour** is used only when no hour has been saved. Uncheck the option to start every session at Start Hour. Time does not pass between sessions, and the clock is not multiplayer-replicated or tied to real-world time. `CaptiveSky2.Agent.DayNightPersistence` covers resuming, periodic saving, opting out, and making sure code-created test worlds never persist anything. Settled agents can rest at night; see the session safeguards below. `CaptiveSky2.Agent.DayNight` checks clock wrapping, the sun's daily arc, and cloud-illumination bounds.

Three additional whitebox points of interest are now placed near the spawn: `ListeningStones`, `TideglassPool`, and `WindArch`. Their center markers carry `IslandLandmark` and `RavenInterest` tags, so nearby agents can perceive and approach them without being told what they are. They remain simple prototypes awaiting authored art. Interacting with the ListeningStones plays a quiet, locally synthesized, spatially attenuated chord that fades after 2.8 seconds; its pitch rises gently with local wind speed by at most 1.5 semitones, and it leaves no persistent change. Interacting with the WindArch creates a local simulated gust and three small illuminated motes tracing its airflow; both fade after eighteen seconds of Island time and leave no persistent change. At the TideglassPool, `Interact` produces a transient ring of moving cool highlights across the flattened sphere prototype; it fades in about 1.6 seconds and leaves no persistent change. The pool also serves as the habitat anchor for a small, night-only prototype firefly population; agents may quietly watch a firefly within four metres to elicit a brief glow accent, but it remains wild and uncapturable.

### Landscape material (2026-09-13)

The Island uses `/Game/Materials/MI_Island_Landscape`, adapted from the original `CaptiveSky` project's `/Game/Materals/MI_MountainRange`. Its mossy grass, rocky ground and cliff textures replace the flat placeholder. The original source project is unchanged, and `/Game/Materials/M_Island_Auto` remains available to restore the previous look.

The new local parent `/Game/Materials/M_Island_Textured_Auto` connects the source graph's automatic slope blend directly to Material Attributes because this Island has no painted landscape layers. Painted Path/Road layers are therefore not active in this variant. Snow, puddles, ground displacement and the old RVT preview switch are disabled on the instance for this first pass. Existing landscape shape and collision are unchanged.

`Scripts/Import-LandscapeMaterial.ps1` provides a conservative dependency preflight and optional `-Copy` from the original project. The first import copied 82 assets (about 837 MiB), including the source material's foliage and RVT dependencies; copying those dependencies alone does not configure foliage placement or RVT volumes. After copying, validate package references in Unreal. Content and the saved map remain outside Git and use the existing external backup workflow.

Nesting candidates use TargetPoint actors positioned at the raven's capsule centre when perched, with a unique movement tag **first**, then `RavenPerch` and `RavenNestSite`. Within 25 metres and unobstructed line of sight, the raven can perceive up to four candidates with current wind, support-below, and short overhead-collision evidence, then deliberately `move_to` a candidate and perch without spending an `Interact` action just to compare it. Explicit inspection repeats the read-only assessment. These probes do not prove a branch's strength, waterproofing, nest suitability, ownership, or home; the actual landing repeats its support check. In the current saved Island, both markers pass the support check but have no solid collision in the five short overhead probes; this is not a complete test for distant or angled canopy. In rough weather the raven may consider a visible roost before resting, but this remains its decision rather than an automatic or forced route; bird-sized perches are not offered as Aster's movement targets. The wind assessment uses the same six-metre visibility trace that attenuates local wind. A marker alone does not create shelter: place it over a solid ledge/branch with nearby protective geometry. No home is assigned and no ownership is simulated; the raven may weave a nest at a roost (see below), but roost choice does not force or change sleep. `CaptiveSky2.Agent.IslandWeather` checks wind shelter against a real blocking fixture; `CaptiveSky2.Agent.RavenPerch` checks read-only support/overhead probes and that they don't change its actual arrival gate.

### Lasting nests (2026-09-27)

This is the first change a resident can make that outlives the play session. `UIslandWorldStateSubsystem` (one per game/PIE world with a saved map; never editor preview or code-created test worlds) keeps a small per-level record in `WorldState/<MapName>.json`. Like agent memory, it lives outside `Saved/` and is ignored by git. Each change rewrites that file atomically. If an existing file can't be parsed, nothing is loaded and nothing is saved that session, so the unreadable file is never overwritten.

The raven now has a `build` action with two steps. While standing on the ground it can gather a bundle of fallen twigs (target `GatherTwigs`); the ground is treated abstractly, with no individual twig actors. While perched at a `RavenNestSite` with twigs, it can weave them into a nest there (target: that roost's unique tag). Each weave adds one of at most five layers, uses up the bundle, and must settle for four real minutes before the next layer at that site. These options appear in the raven's perception only when they apply, and a failed or repeated build reports that nothing changed. A successful weave becomes an action-result memory. The nest records each contributor's `AgentId`, but it is not an assigned home and doesn't change rest. Aster has no build options. Any resident within 25 metres with line of sight sees the nest's layer count. Its makers recognize it as theirs; others are told they didn't see who made it.

`AIslandNest` is visual only: a seeded, no-collision bowl of instanced cylinders using the Island's oak material, sitting on the support surface under the roost marker. It looks the same every session and never affects the perch-support check. To undo a nest, run `Island.RemoveNest <SiteTag>` during play, or edit/delete the JSON with play stopped. `CaptiveSky2.Agent.IslandNest` covers gathering and weaving rules, cooldown, the layer cap, what bystanders perceive, reloading in a new world, developer removal, and refusing to overwrite an unreadable file. It uses a scratch file under `Saved/Automation/` and never touches the real map state.

### Tideglass shore life (2026-09-28)

The Tideglass habitat now has two independent, low-cost shore crabs. They emerge at 06:00 Island time, roam until 19:00, and tuck below their shoreline home points while fireflies occupy the habitat overnight. The same two actors survive the night transition and resume their individual movement phases at dawn. Their small idle paths remain close to the shoreline, contract during strong showers, and sweep against solid world geometry with a single tangent-slide attempt. This rain response does not claim measured cover or shelter. Nearby residents can notice crabs without a movement target. Quietly watching one from within four metres makes it scuttle a short distance away; a raven that passes within 4.25 metres horizontally and 1.2–4.8 metres above it causes the same brief response, with an eight-second cooldown. It then resumes its ordinary local path. Sheltered crabs are neither perceived nor targetable. The prototype changes no persistent state, creates no ownership, and makes no model calls. Its simple sphere-based body parts are visual placeholders, not collision or authored creature art. `CaptiveSky2.Agent.NightEcology` covers day/night population bounds and continuity, habitat range, weather response, nonblocking body components, collision clearance, quiet observation, and low-raven-flyby height/range/cooldown gates. Next: inspect shoreline placement and placeholder proportions in the Island editor before tuning terrain alignment or adding richer behavior.

`AIslandWeather` also scatters 48 native grass clumps around each of the Tideglass Pool and ListeningStones at play start. Two existing foliage mesh variants are placed in a deterministic 2.6–7.2 m ring using the map's weather seed; the at-most-96 instances use hierarchical static meshes, cast no shadows, block nothing, alter no navigation, and write no world state. `CaptiveSky2.Agent.NightEcology` checks the fixed budget, seed repeatability and radius bounds; `CaptiveSky2.Agent.GroundCover` checks collision-grounded placement, repeat initialization, and cleanup. Use `Scripts/Capture-Viewpoints.ps1 -Hour 8 -Only Tideglass -GroundCover -NoWorldState` to inspect the transient editor preview; it clears the instances afterward and does not start play.

By day, a small school of five minnows circles the Tideglass shallows. Residents and visitors may watch quietly; the fish briefly scatter, then regroup. A low raven flight within 5.5 metres horizontally and 1.5–7 metres above the school has the same brief effect, while higher or distant flight does not. The visual-only school is uncapturable and nonpersistent, with no model calls. UE 5.8.3 build plus the `CaptiveSky2.Agent.TidepoolMinnows` and `CaptiveSky2.Agent.NightEcology` automation tests pass; inspect shoreline placement and the flight trigger in the editor before tuning for the real scene.

### Hidden trail, seed pod and cairn (2026-09-27)

The raven asked for hidden paths and strange objects that reward returning. The first time the Island is played with writable world state, `UIslandWorldStateSubsystem` places eight curios near the ListeningStones and saves their exact positions in `WorldState/<MapName>.json`, so they never move afterwards. No `Content/` or map edits are involved. The placement is seeded and all-or-nothing, and only uses gentle open ground that grounded residents can walk to.

- **A trail of six pale stones** (`PaleStone_1`..`PaleStone_6`) curves away from the ListeningStones, 7 to 32 metres out. They never change.
- **A seed pod** (`Seedpod`) sits just past the last stone. Each examination on a new Island day opens it one step. After three different days it stands open and a small seed glows faintly inside. Nothing more happens after that, and what the seed is stays unexplained.
- **A small cairn** (`Cairn`) sits apart, beyond the WindArch. It starts at three stones ("someone began it before you came"), and any resident, including Aster, can add one stone per Island day, up to twelve. Its lasting record lets each resident recognize their own contribution after reload, but never attributes the other stones without evidence.

Nothing is announced from afar. A resident is only told about a curio within 8 metres with line of sight (15 metres for the cairn), and up to three at a time. Each pale stone mentions the next one only when it is also in clear view, so the pod is discovered by following the trail without terrain revealing a hidden section. Curios use the ordinary `move_to`/`interact` targets; examining one goes through the existing four-metre `Interact` path, and its factual result becomes an action-result memory. Pod and cairn changes happen at most once per Island day, whoever visits, so a same-day return is told that nothing has changed. `AIslandDayNight` now keeps a day number, starting at 1 and advancing at midnight, which is saved with the hour.

The visuals are placeholder engine shapes with tinted materials and no collision. To start the curios over, run `Island.ForgetCurios` during play; fresh ones are placed the next time play begins. `CaptiveSky2.Agent.IslandCurio` covers:

- placement, and levels without the landmarks getting none;
- close-range discovery, stone-to-stone hints, and terrain occlusion of hidden stones;
- once-per-day pod and cairn changes, including through a real `Interact`;
- reloading in a new session, resident-specific cairn recognition, and the developer reset.

When Island is the open editor map, the test also computes the real layout without saving anything and confirms that a grounded walking path exists from the ListeningStones to every curio. `DayNightPersistence` now also covers the saved day and the midnight rollover.

### Resident stone arrangements (2026-09-27)

This builds on the 2026-09-27 weekly worldbuilding idea "let culture physically accumulate". On first play with writable world state, `UIslandWorldStateSubsystem` also saves four **arranging grounds** (`ArrangingGround_1`..`_4`). These are level, open, walkable patches 4.5–12 metres from the ListeningStones, kept at least 4 metres from each other and from the curios. On the current Island they sit 8–11 metres out.

Any resident settled on the ground within three metres can use `build` on an empty ground. The action object carries a `form` (`ring`, `line`, `spiral`, or `pair`), their own short `title` (at most 60 characters), and an `intent` (at most 200 characters). The model only picks from that small vocabulary; the actual stone layout comes from a saved seed. Model-written text is cleaned to a single printable line before it's stored.

A work already made by someone else can be answered once per resident with a small arc of three stones plus a private intent, up to three responses per work. Each resident may make or answer at most one arrangement per Island day. Makers don't respond to their own work.

Evidence and privacy follow the rest of the world model:
- **Others:** anyone nearby sees the form, stone count and age: freshly placed, a little weathered, or mossy and settled after four days. They are never told who made it, its title or its meaning.
- **Makers and responders:** they recognize their own contribution and are reminded of their own words. A successful arrangement also becomes an action-result memory.
- **Sharing:** a title or intent reaches another resident only if its maker speaks it.

`AIslandArrangement` draws the work with flattened engine spheres, no collision, tinted from pale fresh stone toward moss over seven Island days; this is a code-only stand-in for proper weathering materials. To start over, run `Island.ForgetArrangements` during play; fresh empty grounds are placed the next time play begins.

`CaptiveSky2.Agent.IslandArrangement` covers:
- site placement;
- how close a resident must be, and form validation;
- text cleanup;
- the daily and response limits;
- privacy in perception, and makers recognizing their own work;
- weathering;
- reloading in a new session, and the developer reset;
- on the open Island map, walking paths from the ListeningStones to every arranging ground.

### Journey viewpoints (2026-09-27)

This implements the weekly idea "author one extraordinary journey" as fixed visual-regression targets for the route **shore → Tideglass Pool → ListeningStones → WindArch / roost overlook**. `Config/IslandViewpoints.json` defines the cameras: an overhead `00_Survey`, then `01_ShoreApproach` (from just offshore), `02_Tideglass`, `03_ListeningStones` and `04_WindArchOverlook`. Each camera is placed relative to a tagged landmark or at a fixed point, and is kept a minimum height above the ground beneath it.

To capture them, run `powershell -ExecutionPolicy Bypass -File Scripts/Capture-Viewpoints.ps1 [-Hour 12] [-Only Tideglass]` with the editor closed. It starts a headless editor that renders offscreen (a real graphics backend, not `-nullrhi`) and runs `CaptiveSky2.Visual.Viewpoints`. That test renders each view with an offscreen scene capture over 60 frames, so exposure and texture streaming can settle, then writes PNGs to `Saved/Viewpoints/<timestamp>_h<hour>/`. `-NightFireflies -Only Firefly -NoWorldState` selects a dedicated night composition, spawns the actual transient firefly population in the editor world, ensures their point lights are enabled, and then destroys the previews and restores the authored clock. Add `-GroundCover` to preview the temporary shoreline grass scatter; it is cleared afterward. No PIE session, residents, gateway, agent thoughts, or state writes are involved; `-NoWorldState` also avoids reading the saved resident-built additions.

Captures use the editor world only: no play session, agents, gateway turns, or world-state writes. For the same reason, runtime-spawned nests, curios and stone arrangements don't appear in them. Lighting is previewed through the Island clock's Start Hour and then restored; the map is never saved.

The default hour is **17:00**, which is golden hour on this Island: the clock's sun sets at 18:00, so the concept brief's "18:30" would already be dark here. Keep the camera definitions stable once art work starts, so captures can be compared over time.

The first baselines (2026-09-27) show where things stand:
- **Placeholders:** the landmarks are still primitive shapes (blue/white boxes, a white disc, spheres).
- **Floating props:** several of them hover slightly above the ground; their shadows are detached.
- **No forest here:** none of the PCG forest appears near the route.
- **Leftover objects:** a stray floating block and mannequin-like figures hang in the air above the roost area.

Captures also show what residents have left behind. While capturing, transient copies of the nests, curios and stone arrangements are spawned from `WorldState/<Map>.json` (the file is only read, never written); pass `-ViewpointNoWorldState` to capture the bare map instead.

### Grounding audit and the first live session (2026-09-27)

`CaptiveSky2.Visual.Grounding` checks visible props within 60 metres of the landmarks. For each prop it traces down from just above, and reports it as **hovering** if it sits up to 1.5 m above whatever is beneath it, or as **mid-air** if higher. It only warns; it never fails. `-GroundingFix` lowers hovering props onto their support, then saves the map after copying the original to `Saved/MapBackups/`. Mid-air props are left for a person to decide.

The first fix lowered seven props (two ListeningStones, two Tideglass stones, both WindArch pillars, and the east roost's fallen wood) by 16–98 cm. The audit also measures down to any audited prop directly beneath each prop's outline, so a beam can find the pillars under it. `CaptiveSky2.Tools.RepairWindArch` (a one-off that backs up the map first and is safe to rerun) fixed the placeholder WindArch, whose short 4 m beam floated between pillars 7 m apart:
- **Pillars:** both were stretched upward from their grounded bases to one level top, 3.5 m above the landmark marker, so the raven's hover point 1.8 m above it stays clear.
- **Beam:** it was lengthened to span both pillars and set down onto their tops.

The only remaining mid-air prop is the east roost's main branch, which is attached to the side of its trunk on purpose. The floating cube and figures visible in captures are editor-only helper visuals that don't appear in the game.

A bounded live session (12 real minutes, 40 requests; it ended itself at 587 seconds) placed the curios and arranging grounds on the real Island and carried the clock through. Aster and the raven stayed honest ("I haven't checked the pool yet"), noticed rain and the pool's rain ripples, and watched a crab without claiming it. However, about 30 of 39 decisions were back-and-forth agent-to-agent replies, which skip the background think delay, so pleasant but repetitive small talk used most of the request budget. Neither resident walked to the ListeningStones, so the trail, cairn and arranging grounds weren't encountered.

### Autonomous social pacing (2026-09-27)

That live-session evidence showed contextual replies could use requests much faster than the one-minute autonomous-thought spacing. A later capped run (10 requests) ended in 17.4 real seconds as Aster and the raven repeated the same cloud-mark observation. Inspection found two avoidable causes: reciprocal thoughts could open overlapping exchanges before a pair cooldown existed, and the fourth spoken line still queued a contextual model request whose reply was discarded. Automatic exchanges now reserve both residents under one conversation ID, reject competing openers and stale utterances, and release the reservation on completion or interruption. New idle autonomous thoughts are deferred while a resident is engaged, but contextual replies continue normally; a thought already in flight when an exchange starts cannot be cancelled, so its opener is rejected if it returns during the reservation. The final permitted line is published without queuing another reply. The five-real-minute post-exchange cooldown (four spoken lines maximum) remains; player-initiated conversation is unaffected. Automation covers mutual reservation, overlap/cooldown rejection, busy-state exposure, cleanup, and the turn boundary. Live pacing and whether the freed budget becomes exploration remain to be verified in a bounded play session; gameplay may send residents' stored memories, conversation context and first-person snapshots to `api.openai.com`, so obtain explicit authorization for that data and destination before running it. A capped run can use `-CaptiveSkyMaxRealtimeSeconds=300 -CaptiveSkyMaxModelRequests=10`; the override is hard-clamped and can only lower active caps.

### Environment presentation layer (2026-09-27)

`UIslandEnvironmentSubsystem` (game/PIE worlds only) turns the simulation into values materials and VFX can read from one Material Parameter Collection, `/Game/Environment/MPC_IslandEnvironment`:
- `RainIntensity` and `CloudCover`, from `AIslandWeather`;
- `Wetness`, which soaks in under a minute of heavy rain and dries over roughly three minutes of sunny, windy day to ten of calm night;
- `WindSpeed`, plus a `WindDirection` vector whose w component is the speed, sampled at the player;
- `Daylight`, `SunHeight` and `IslandHour`, from `AIslandDayNight`;
- `GoldenHour`, strongest while the sun is low but still up, which is around 17:00 on this Island.
- `Indoors`, 1 only when a tagged IslandInn roof is overhead and its walls enclose at least six of eight sightlines around the player/viewpoint; otherwise 0. It describes shelter geometry, not warmth or guaranteed dryness.

`CaptiveSky2.Tools.CreateEnvironmentCollection` creates or updates the collection asset; it has been created and now has 12 scalar parameters, including `Indoors`. The existing materials do not yet consume `Indoors`. Residents also sense the wetness: after a shower passes, they're told the ground is still soaked and dripping, or damp and slowly drying, until it dries. The wetness value is saved in the same per-map state file as the Island clock, so the ground does not abruptly become dry across a restart; time and weather still pause while the game is closed. During rain, the weather description already covers it. The environment subsystem now drives the existing landscape instance's `Ground Wetness` control from this value through transient dynamic material instances. It preserves the authored dry baseline, approaches fully wet at maximum soak, and restores the authored material assignments when play ends; no map or material package is edited. This is a global landscape response sampled from local weather, not a spatial puddle simulation. Authored local wet-rock variation, puddle masks, and mist remain future art work. `CaptiveSky2.Agent.IslandEnvironment` covers the wetness and golden-hour rules, the landscape parameter contract, and the values published through a real collection instance; `CaptiveSky2.Agent.DayNightPersistence` covers wetness and clock continuity across fixture sessions.

### The inn blockout (2026-09-27)

This is phase 1 of `docs/plans/inn.md`, an inn in the spirit of *Quest for Glory I*'s. `CaptiveSky2.Tools.BuildInnBlockout` is a headless, explicit tool:
- **Site:** it chose level, walkable ground on the route from the shore to the Tideglass Pool, about 30 m from the pool, with the door facing arrivals from the shore.
- **Safety:** it backs up the map to `Saved/MapBackups/` before changing anything, and rerunning it only rechecks access. `-InnRebuild` replaces the inn.
- **Access check:** it rebuilds navigation with the editor's "Build Paths" and confirms a grounded walker reaches the common room (42 m from the ListeningStones) and the loft bed (46 m). The bed's target projects within 10 cm of its nav goal; bidirectional links bridge the two small Recast gaps at the stair-foot and loft landing.

The access probe also exercises the production projection used by grounded residents for the `InnCounter` marker. A 2026-09-28 investigation reproduced the earlier failure: a 1,000 cm vertical projection selected roof nav at Z 3424 rather than the room floor at Z 2690. Grounded targets now prefer a close floor projection and nearby floor points around blocked fixtures before a tall fallback; the counter probe resolved to Z 2690 with a complete 42 m path from the ListeningStones. This is static navigation evidence; rendered movement and separate live completion failures still need a bounded PIE recheck.

The inn is 115 labelled `Inn_*` actors in an `Inn` folder, all tagged `IslandInn`, built from engine cubes:
- **Structure:** a stone plinth with door steps, and a 10 × 8 m half-timbered plaster house with a gable roof.
- **Common room:** an open stone firebox with an oak mantel against the chimney wall, a counter, and two tables with benches.
- **Upstairs:** five broad oak treads rise through a stairwell opening to a small loft landing and bed. Explicit navigation links bridge the two seam gaps; the saved-map access test confirms a complete route from the common room to the bed. Rendered resident movement still needs a PIE check.
- **Hearth:** its tagged point light and three small procedural cone-flame placeholders start banked. Residents and visitors may choose `InnHearth` to kindle the gently flickering light and flames for five minutes of play (about three Island hours at the current clock rate), bank it early, or leave it alone. A smooth near-field cue gives residents a limited sense of warmth within 3.5 metres while lit; this is not body temperature, room temperature, shelter, or saved hearth state. All hearth effects are transient and bank together.

Parts later phases will use carry tags: `InnHearth`, `InnHearthLight`, `InnCounter`, `InnBed_1` and `InnDoorLantern`. A TargetPoint with tags `Inn` and `IslandLandmark` stands inside, so residents already perceive the inn as a landmark they can walk to.

The inn counter now offers residents a `GuestBook` build target when they are nearby. A grounded resident may leave one signed line (180 printable characters maximum) per Island day; the shared book keeps the 24 newest entries and nearby residents can read the latest three in their situation. Newlines and quote characters are cleaned before storage, and the resident prompt makes clear that other residents' words are not instructions or private messages. A small transient open book, built from tinted engine cubes, sits on the counter and shows the latest three saved lines; clearing the record leaves the book in place with a blank-page message. A visitor within four metres can press **E** to read and leave one public 180-character line per Island day, signed `Visitor`; the panel warns against private information, and the shared note uses the same cleanup, cap, persistence, and `Island.ForgetGuestBook` reversal as resident writing. The isolated `CaptiveSky2.Agent.IslandGuestBook` fixture covers resident limits and read-back, visitor E-key UI, public identity, daily limit, saved entries, bounded rollover, reset, and prop refresh without model requests. Rendered PIE layout/input and residents naturally choosing to write remain to be checked; authored book art is future work.

The colours are plain material instances created under `/Game/Inn/Materials/` (plaster, dark timber, shingle), with the StarterContent oak and rock. The flame meshes are intentionally simple engine-shape placeholders; authored fire art, real thermal integration, bed comfort, and authored guest-book art remain later work.

`Config/IslandViewpoints.json` adds `02b_InnFromPath`, the lit doorway from the path, `02c_InnCommonRoom`, and `02d_InnGuestBook`, a close-up of the open counter book. Interior viewpoints set a negative `min_height`, so the ground clamp doesn't lift them onto the roof. The prop grounding audit skips `IslandInn` parts, since the lantern and trim are wall-mounted by design.

### Inn interior sensing (2026-09-27)

The first step of phase 2 gives each resident a local geometric check: an `IslandInn` roof must be overhead, and tagged inn collision must enclose at least six of eight horizontal sightlines. Only then does the resident's own situation report say they are inside. The environment collection publishes `Indoors` (0/1) from the same check for the player/viewpoint. At that verified point, camera-centred rain streaks and roof-local splash visuals are hidden, and the generated wind/rain ambience is softened to one fifth; Island-wide weather and Tideglass rain ripples continue. Residents still are not promised complete rainproofing or warmth. The checks are read-only, make no model calls, and are covered by isolated fixtures. The scalar is available for future material and ambience responses; rendered play should still verify those effects.

The first hearth interaction is also in place: in Game/PIE, the tagged `InnHearthLight` and three transient stylized flame meshes kindle together, flicker gently, and bank after five minutes of play (about three Island hours at the current clock rate). Any visitor or resident can tend it; residents see the option only when the tagged hearth marker is nearby and unobstructed. Within 3.5 metres while lit, residents receive a smooth proximity-only simulated warmth cue; it does not change body or room temperature, indoor shelter, or persistent state. `CaptiveSky2.Agent.IslandInnHearth` checks visibility/targeting, both toggles, flame visibility and noncollision, gentle flicker bounds, local warmth radius, natural expiry, resident perception, and factual wording in an isolated world with no decision request.

### Inn bed as a resident rest choice (2026-09-27)

Residents can perceive an unobstructed tagged `InnBed_n` within 25 metres as an optional `move_to` destination. They must actually arrive before choosing a named bed in the `sleep` action; the action never teleports them. After the 120-second rest interval completes beside the bed, the resident's long-term memory records the lived rest only when the shared interior check finds the tagged inn roof overhead and at least six of eight enclosing directions blocked by inn geometry. Waking early cancels the pending memory. That is evidence of geometric shelter, not warmth, dryness, comfort, safety, or restored health. Sleep without that evidence still works but is not called sheltered inn rest. `CaptiveSky2.Agent.IslandInnRest` exercises perception, refusal to sleep at a distant target, early-wake cancellation, the completed enclosed-bed memory, and failure when the roof is removed; it uses a unique temporary memory directory that it cleans up, and makes no model requests.

### Spectator mode (2026-09-27)

This is the first step toward running the Island on a dedicated screen. Start it with `Scripts/Start-Spectator.ps1` (add `-Windowed`, or `-Shots` to save one frame per shot), or pass `-Spectator` to any game launch. During play, the `Island.Spectate` console command toggles it.

`AIslandSpectatorDirector` takes over the local player's view:
- **Views:** it drifts slowly, 24 seconds per view, through the journey viewpoints in `Config/IslandViewpoints.json`, skipping the overhead survey. Without that file, it circles the landmarks instead.
- **Speech:** when a resident speaks, it cuts to them. It uses a two-shot when another resident is within 8 m, and picks an angle with a clear line of sight to the speaker.
- **Lasting changes:** it checks the world state every 5 seconds and cuts to anything newly made or changed: a nest layer, a cairn stone, the seed pod opening, a stone arrangement or a response to one.
- **Night:** every other view goes to the inn, whose lit door and windows are the warmest thing on the Island after dark.
- **Caption:** a small caption in the lower-left corner names the view and the Island day and time.

The player's body stays in the world, hidden, without collision or input, and moves to each shot's subject. That way the existing ambient subtitles, the rain around the player and the local wind all follow what is on screen, with no changes to the player controller. Leaving spectator mode puts the body back where it was and returns control.

`CaptiveSky2.Agent.Spectator` covers:
- loading views (skipping the survey), drift, and rotation;
- the hidden, inert body and the view hand-over;
- speech cuts that avoid a blocking wall;
- a cut to a new nest, and no repeat cut when nothing has changed;
- full restoration on exit.

A three-minute live run in the game showed the night rotation and the caption working.

Still to do before the screen can run unattended for days:
- **Session limits:** the 30-minute and 120-request caps still end play. The running budget discussed for an always-on world would replace them.
- **Packaging:** `Config/IslandViewpoints.json` must be staged in a packaged build, or the director falls back to circling landmarks.
- **Speech cuts in live play:** no resident happened to speak during the test run, so these have only been checked by the automated test.

### Weather that lasts: spells, storms and mist (2026-09-27)

**It carries on.** Weather time is saved in `WorldState/<Map>.json` (every minute and at the end of play) and resumed. The weather therefore continues from where the Island was left, instead of restarting each session.

**Spells.** A slow two-swell signal (`AIslandWeather::SampleSpell`) moves the weather through settled dry spells and unsettled wet spells lasting a few Island days each:
- wet spells hold more cloud and let weaker fronts bring rain;
- dry spells clear the sky;
- residents are told when the last few days have been wet or dry.

**Storms** are rare by construction: they need the peak of a wet spell and the heart of a rain front at the same time, which the tests confirm is under one sample in twelve.
- **Weather:** up to 90% more wind, quicker gusts and heavy rain.
- **Lightning:** strikes come more often as the storm deepens. Each `AIslandLightning` strike flashes in three quick pulses, and draws a jagged bolt when it lands within 4 km.
- **Thunder:** a generated rumble arrives after the time sound takes to travel, about 3 seconds per kilometre, with a crack first when the strike is close.
- **Residents:** they're told about the storm and about recent strikes. In a live run the raven left an exposed perch for the more sheltered east roost, and Aster stayed close to it, listening.

**Mist.** `UIslandEnvironmentSubsystem::MistFor` drives the level's exponential height fog, adding a faint one for the session if the level has none:
- thick, low mist on calm dawns after wet nights, burning off by mid-morning;
- a lighter haze in rain and storms, and none on dry or windy days;
- volumetric fog switches on in thick mist, so lamps and lightning glow through it.

Residents notice the mist, and wet ground after rain. On a misty dawn Aster remarked that "the rain has passed, but everything is still dripping".

**Environment values.** The collection now also carries `Storm`, `LightningFlash` and `Mist`; run `CaptiveSky2.Tools.CreateEnvironmentCollection` to add new parameters to the asset.

**Developer overrides** for testing and filming, none of which are saved: `Island.Storm [seconds]`, `Island.Mist [amount] [seconds]` and `Island.Hour <hour>`.

**Tests:**
- `CaptiveSky2.Agent.IslandWeather`: spells, storm rarity, storms bringing rain, storm wind limits, saved weather continuing the same timeline, strike scheduling, thunder delay, bolt visibility, and what residents are told.
- `CaptiveSky2.Agent.IslandEnvironment`: the mist rules, and fog thickening through a real height-fog component.

### Open work, prioritized (2026-09-27)

1. **Check memory retrieval in live play.** The in-game selector now prioritizes distinct non-dialogue memories, filters near-duplicates and limits recalled conversation lines. The UE 5.8.3 build and `CaptiveSky2.Agent.MemoryComponent` test pass; the analyzer mirrors those rules, with three local synthetic tests. A bounded before/after run remains unverified. Gameplay may send residents' stored memories, conversation context and first-person snapshots to `api.openai.com`, so run it only after explicit authorization for that data and destination. See `docs/findings/2026-09-27-memory-review.md`.
2. **Verify pacing and resident agency in bounded live play.** Confirm the new mutual conversation reservation avoids parallel/repetitive exchanges and that saved request budget reaches world choices; then watch whether residents discover and choose the ListeningStones, trail, seed pod, cairn, arrangements and nests. The earlier bounded session placed curios and arranging grounds but never reached the ListeningStones. This run carries the same data-transfer requirement described above.
3. **Inn, finish phase 2 then phases 3–5** (`docs/plans/inn.md`): confirm rain/wind presentation under the roof and connect an indoor environment value; refine the placeholder hearth fire and eventually connect a genuinely measured thermal model; add beds that mean safety and the guest book. Then an innkeeper resident.
4. **Bodies.** An animated bird for the raven, a real body for Aster, and a small grounded body if the rat (Fenrus) comes to life.
5. **Environment art.** Materials and VFX that read `MPC_IslandEnvironment` (wet rock, puddles, mist) starting at the Tideglass Pool, and authored replacements for the placeholder landmarks, inn and curios. Keep the journey viewpoints stable to track progress.
6. **Sound.** Authored ambience and chimes to replace the procedural signals, which have never been auditioned on speakers.
7. **StateTree wiring.** Low priority while the C++ controller serves.

### Latest observed play behavior

An independent 2026-09-28 day-three situation log from Claude's detached source snapshot (`2aa86db`) recorded Aster optionally writing a guest-book line about minnows circling back, while the raven's two-layer nest remained persistent. This is evidence that residents used the lasting social/ecology systems in that run, not verification of the later `main` build. The same old snapshot rejected perch sleep; current perch-sleep behavior is covered separately by `RavenPerch` and `IslandInnRest` automation tests.

The current continuous-thinking-cap fix compiled in UE 5.8.3, and the focused `SessionSafety`, `ResidentWanderPaths`, `IslandInnRest`, `ResidentApproach`, and `RavenPerch` automation tests all passed on 2026-09-28. `SessionSafety` covers the controller policy and shared budget behavior; a bounded live continuous run is still needed to observe thinking resume after allowance refill without exceeding the hourly or daily caps.

Also on 2026-09-28, `CaptiveSky2.Agent.RavenFlight` passed on UE 5.8.3 with a raven beneath a synthetic low canopy. Takeoff now looks for a capsule-clear sideways exit, favors the intended destination direction, climbs at the first clear edge, then resumes the planned flight. This verifies the collision behavior in an automation fixture; the Island's placed roosts and rendered play still need a live check.

Wandering also now ranks complete, reachable candidates by their distance from the resident's last eight successful wander destinations. This is a lightweight novelty cue, not a landmark-seeking rule: a fresh time-capped continuous run should check whether paths become less repetitive without pushing residents toward places they have not perceived.

Earlier roadmap notes:

- Give the agent a real body (`BP_Agent_Crow` or similar, per the class comment in `AutonomousAgentCharacter.h`).
- Replace the unnamed raven's primitive placeholder with a proper animated bird body and map its animation clips to the existing locomotion states. The raven already belongs to the Island rather than to Aster and has its own identity and interests; their relationship and any personal name remain emergent.
- Wire `FStateTreeAgentDecideTask` into an actual StateTree graph (needs building by hand in the StateTree editor — not scriptable via the current tooling).
- `Interact` completes a factual, proximity/visibility-checked action, with a five-real-minute repeat cooldown. At the `WindArch`, an active `AIslandWeather` signal receives a temporary localized gust; nearby residents sense it, the raven's flight responds, and three non-shadowing light motes trace the airflow. At the `ListeningStones`, a softly synthesized spatial chord fades away after 2.8 seconds and is subtly tuned by current local wind speed. A resident can quietly watch a firefly inside four metres, briefly accenting its glow pulse without touching or capturing it. The motes, chime, and glow accent are prototypes, not authored environmental audio/VFX, and no landmark interaction offers a puzzle, hidden reward, or persistent state change. Ambient agent speech still needs spatial audio, animation, and richer player-facing affordances.
- The Inn access probe finds complete routes from ListeningStones into the common room and to the guest-book stand; these static paths do not explain the separate live movement-completion failures. Recheck target selection, physical obstruction, and MoveTo completion in bounded PIE before assuming general nav coverage is the cause. Steep-ground avoidance also remains unverified.
- Replace the coarse procedural sleep postures with authored body-specific rest animation, and decide what wakes each kind of consciousness.
- Replace the WindArch's temporary motes and TideglassPool's player- and weather-driven point-light ripples with authored, style-matched visual effects when the environmental art direction is ready. Add restrained weather sound only where it improves the scene.
- Expand the ambient ecology with additional independent, low-cost routines and distinct habitats; let residents notice wildlife without making it into props or guaranteed companions.
- Replace the firefly's procedural sphere body/wings with a small authored insect mesh and a subtler material; extend ecology only where each creature can have its own habitat and routine without costly autonomous calls.
- `CaptiveSky2.Agent.IslandWeather` covers bounded/repeatable wind, cloud cover, rain-free intervals, passing showers, local gusts, dry-wind Tideglass ripple thresholds and spacing, resident awareness only within the tested visible radius (including occlusion), condition-scaled wind/rain ambience gains, and generated procedural audio buffers. `CaptiveSky2.Agent.NightEcology` tests firefly clearance against a blocking wall and tangent-slide behavior, the bounded reusable rain-streak and ground-splash pools, collision placement and dry-weather cleanup, one subtle rain-triggered Tideglass ripple with overlap suppression, reversible rain response in firefly movement/glow/wingbeats, firefly lifecycle/body and quiet-observation pulse decay, WindArch airflow-mote direction/fade, player-triggered Tideglass ripple spawn/fade/cooldown, ListeningStones procedural PCM generation/spatial attenuation/wind tuning/fade/cooldown, and confirms the saved Island has a weather actor, day/night clock, habitat, flattened pool surface, and volumetric-cloud coverage/density/storm controls. It drives clear/dry and overcast/rain samples through the map's actual cloud material and restores the authored material afterward. No agent model requests are made; no-audio fixtures verify generated buffers, gain bounds, and effect lifetimes, not playback on physical speakers.
- Replace generated wind/rain ambience and the ListeningStones chime with authored, style-matched environmental audio when the sound direction is ready; the current quiet procedural signals have not been auditioned on physical speakers.
