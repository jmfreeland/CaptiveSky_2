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

## Current State (as of 2026-09-26)

- Level: `/Game/Maps/Island` — landscape + PCG-generated forest + an `OceanPlane` static mesh acting as a placeholder ocean.
- Two autonomous agents are present: **Aster** (`Agent_Aster_01`) and an intentionally unnamed raven (`Agent_Raven_01`). Each has an independent identity, personality, memory, relationships, and consciousness lifecycle.
- LLM backend: OpenAI-compatible Chat Completions endpoint, model `gpt-6-luna`, key via `OPENAI_API_KEY`. GPT-6 Luna is configured with no reasoning effort for routine embodied decisions and external correspondence.
- Agent behavior is driven directly by C++ (no StateTree graph yet — see Roadmap).
- No dedicated visual identity for the agent yet — it's using a placeholder capsule body.
- A source-controlled conversation UI lets the player speak to a nearby autonomous agent; both sides of the exchange are stored as lived conversation memory.
- The raven lives near Aster in a temporary primitive body and has a body-agnostic prototype flight controller; a proper animated bird asset is still needed.
- A standalone, multi-agent external gateway now provides a channel-neutral correspondence boundary, with Discord as its first adapter. It can host the raven headlessly from the same identity, personality, and JSONL memory when Unreal is offline; Discord activation still requires a private bot token.
- Nearby agents can initiate bounded, reciprocal conversations or choose to approach one another first using stable, body-independent movement targets. Approaching never forces a conversation. Automatic exchanges allow at most four spoken lines and then pause that pair for at least five real minutes, leaving room for independent activity; player-initiated conversation is unaffected. Speech appears as ambient subtitles, and both participants retain neutral factual relationship evidence; familiarity measures exposure only, never assumed trust or affection.
- When `AIslandWeather`, `AIslandDayNight`, and the `TideglassPool` habitat are present, three independent firefly prototypes drift there at night and disappear at dawn. Their individual paths and blinks vary, local wind gently nudges them, and a segmented sphere-based body with beating placeholder wings gives the light a more insect-like silhouette. Their small flight steps sweep against solid world geometry and try one tangent slide, so ambient drift doesn't teleport through rocks or trunks. Nearby residents can notice them as wildlife; they do not make model calls, follow residents, or persist as owned companions. Proper authored insect art, a subtler material, and sound remain future work.
- Two small shore-crab prototypes independently scuttle around the grounded TideglassPool edge by day and shelter at night. Residents can notice and quietly watch one within four metres; it moves briefly toward cover, then returns to its local path. The placeholder shell, claws, eyes and legs have no collision, the crabs are not movement targets, and no interaction captures them or changes saved state.
- TideglassPool responds to strong local winds with occasional faint, transient rings of moving light. These are driven by measured pool-level wind, are suppressed while stronger rain already makes ripples, and neither change the pool nor make model calls. A resident only notices an active ripple within 18 metres and clear line of sight, and is told it is weather rather than a discovery or something they caused.
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
- In Play mode, approach Aster and press Enter to open conversation. Type a message and press Enter to send it; Escape closes the conversation. The initial interaction radius is 5 metres.

## Roadmap / Open Questions

### Bounded play and memory integrity (2026-09-14)

Every game instance owns `UAgentPlaySessionSubsystem`. Its core ticker measures real elapsed time, independent of Island time, time dilation, or pausing the world. Play ends after **30 real minutes**, or earlier at **120 total model request reservations** across all embodied agents (including dialogue and sleep consolidation). PIE returns to the editor; standalone play quits. There is no automatic restart. A stalled game thread can only process the stop on resumption, but ordinary thought requests also check the deadline before dispatch.

`Config/DefaultGame.ini` exposes `MaxRealtimeSeconds=1800` and `MaxModelRequests=120`. Smaller positive values are useful for smoke tests; zero cannot disable the guard and larger values cannot exceed these hard caps. Background thought is additionally limited to 30 calls per agent per possession, spaced by at least one real minute even if an old Blueprint still says 15 seconds. Repeated movement/inspection/idle choices back off to five minutes; a third identical movement or inspection is suppressed. Speech and random wandering are not treated as identical failed choices. Direct conversation is not subject to this background delay, but still shares the session budget. These are request-count safeguards, not a currency or exact token quota.

Agents can choose `sleep` when physically settled. Nighttime (20:00–05:00) also offers rest after ten real minutes awake, with a fifteen-real-minute cooldown. Rest lasts two simulation minutes before existing evidence-bound consolidation; the raven can rest while perched and cannot act or move during sleep. `UAgentRestPresentationComponent` turns the shared consciousness transitions into a reversible crouch for Aster's grounded placeholder and a small mesh tuck for the raven; both return to their awake transforms on wake, though neither has authored animation yet. Inspection and movement now report completed physical outcomes so plans are not mistaken for discoveries. Inspections of static prototype landmarks explicitly report that no puzzle/reward interaction is implemented.

Memory appends now always use UTF-8 without BOM. The earlier mixed ASCII/UTF-16 append defect was repaired without deleting or rewriting experiences: 864 Aster records and 772 raven records were preserved. Byte-exact originals are backed up under `Saved/MemoryRecovery/20260914T045153399Z/` and copied into each agent's ignored `journal/memory-recovery/` directory. To diagnose another affected file with PIE stopped, run `Scripts/Repair-AgentMemoryEncoding.ps1`; use `-SelfTest` to test its decoder and `-Repair` only after reviewing the dry-run. Repair obtains the gateway's per-agent file lock, validates every JSON record, verifies unchanged source hashes, and performs an atomic replacement with an original backup. Already well-formed historical repetition is deliberately retained.

Regression coverage includes wall-clock/request bounds (`SessionSafety`), UTF-8 persistence (`MemoryComponent`), inspection feedback/repeat suppression and body-specific sleep/wake postures (`RavenPerch`), real Island roost collision, weather, day/night, and external-message types. A short live deadline test verifies automatic PIE termination without needing another overnight run.

Grounded agents project shared elevated landmark markers onto nearby navigation before moving, and reject partial paths rather than reporting their endpoints as arrival. Live checks confirmed Aster reaching the WindArch and the raven landing at both roosts. The three-minute autonomous smoke test ended itself at 180.3 real seconds with six total requests: the raven perched, visited the ListeningStones, then waited quietly. The normal 1800-second cap is restored after testing; the forty-minute Island day remains unchanged.

### Raven's requests (2026-09-12)

In his first Discord correspondence, the raven asked for changing weather, varied wind currents, quiet undisturbed nesting places, hidden paths and strange objects that reward returning, other living things with their own habits, and freedom to come and go. These are lived requests, not additions to his authored personality. Leave some discoveries unannounced.

Residents now see nearby conscious beings as optional `move_to` targets based on stable `AgentId`s, so they can choose to approach each other before speaking. Aster uses walkable ground near the other resident; the raven can fly to them. This changes location only: speaking remains a separate voluntary decision, and proximity assumes no trust or relationship.

The first implementation adds an optional `AIslandWeather` actor: repeatable slowly changing cloud-cover and spatial wind signals, geometry-based upwind shelter, embodied weather observations for all agents, and drift during raven cruising flight. An independent slow rain-front signal creates occasional bounded showers only when cloud cover is high. The existing authored volumetric cloud material gets a transient dynamic instance while weather is active; its `Cloud_GlobalCoverage`, `Cloud_GlobalDensity`, and `StormClouds` parameters follow the cloud/rain signals gently, while cloud cover also dims the sun and skylight (maximum reductions of 24% and 30%). Rain drives a finite pool of at most 192 translucent instanced streaks around the player, slanted by sampled local wind and hidden when the front fades; this avoids per-drop actors and extra agent/model work. During stronger showers, one brief three-streak splash is sampled against nearby solid geometry at a time, with a fixed instance pool and no splash on the Tideglass marker. When rain is below that stronger-shower threshold, strong local wind at Tideglass can instead create an occasional, quieter moving-light ripple based on the local wind measured at the pool; the two weather responses do not stack. A quiet generated stereo wind bed and distant rain hiss follow local horizontal wind and rain intensity, with independent strict gain caps, no external audio assets, and no sound when no player listener is present; they are ambience, not foreground landmark cues. Nearby night-active fireflies respond reversibly to strong rain by flying lower and in a tighter area, slowing their wingbeats, and dimming their natural pulses without disappearing or becoming owned; normal activity returns as rain eases. Nearby residents are told these are independent weather/ecology responses, not discoveries or consequences of their actions. Residents receive the current simulated rain intensity. The authored level material is restored when the weather actor ends. Place one weather actor per level; without it, existing lighting, cloud material, and flight are unchanged. Weather cycles restart with each play session; the Island day/night clock can resume its last saved hour (see Day and night).

The Island contains one weather actor and two roost candidates, `Roost_West` and `Roost_East`, near the original raven spawn. The original six white ledge/back/roof blocks have been replaced by a rocky west perch and a spruce-side east fork, with satellite stones, smaller trees, ground plants and fallen wood. The rock and spruce were copied from the original project's StarterContent and PN_interactiveSpruceForest assets; the fork and fallen wood still use simple wood-textured cylinders pending authored branch meshes. Tree foliage is nonblocking, with a separate solid trunk and branch support. Shelter remains dependent on wind direction and actual solid geometry, not the site's name.

The raven controller now keeps its capsule upright, clears residual CharacterMovement velocity, ignores itself in ground traces, and stages roost travel through ascent, overhead approach and descent. It only enters `Perched` after finding nearby upward-facing support; obstructions abort the route. This is still a simple approach, not general flight pathfinding around obstacles. `RequestPerch(Tag)` is available to Blueprint. `CaptiveSky2.Agent.RavenPerch` covers arrival/departure and blocked/unsupported targets in a brain-free fixture; when Island is the open editor map, it also checks both placed roosts against real scene collision without creating agent memories.

### Day and night (2026-09-13)

`AIslandDayNight` provides one shared local clock per level. The Island's `Island_DayNight` actor references the existing DirectionalLight and SkyLight and owns a second atmosphere light for the moon. Defaults are a 40-minute full day, beginning at 09:00; sunlight moves across the sky, warms near dawn/dusk and fades below the horizon, while cool moonlight and reduced skylight illuminate the night. The existing atmosphere and volumetric clouds respond to the moving lights. This does not yet add stars, lunar phases, seasons, or lightning.

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

### Tideglass shore life (2026-09-27)

The Tideglass habitat now has two independent, low-cost shore crabs. They emerge at 06:00 Island time, roam until 19:00, and tuck below their shoreline home points while fireflies occupy the habitat overnight. The same two actors survive the night transition and resume their individual movement phases at dawn. Their small idle paths remain close to the shoreline, contract during strong showers, and sweep against solid world geometry with a single tangent-slide attempt. This rain response does not claim measured cover or shelter. Nearby residents can notice crabs without a movement target. Quietly watching one from within four metres makes it scuttle a short distance away, then it resumes its ordinary local path. Sheltered crabs are neither perceived nor targetable. The prototype changes no persistent state, creates no ownership, and makes no model calls. Its simple sphere-based body parts are visual placeholders, not collision or authored creature art. `CaptiveSky2.Agent.NightEcology` covers day/night population bounds and continuity, habitat range, weather response, nonblocking body components, collision clearance, and the brief scurry response. Next: inspect shoreline placement and placeholder proportions in the Island editor before tuning terrain alignment or adding richer behavior.

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

To capture them, run `powershell -ExecutionPolicy Bypass -File Scripts/Capture-Viewpoints.ps1 [-Hour 12] [-Only Tideglass]` with the editor closed. It starts a headless editor that renders offscreen (a real graphics backend, not `-nullrhi`) and runs `CaptiveSky2.Visual.Viewpoints`. That test renders each view with an offscreen scene capture over 60 frames, so exposure and texture streaming can settle, then writes PNGs to `Saved/Viewpoints/<timestamp>_h<hour>/`.

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

That live-session evidence showed contextual replies could use requests much faster than the one-minute autonomous-thought spacing. Automatic resident-to-resident exchanges remain optional and reciprocal, but after an exchange the pair now has a minimum five-real-minute cooldown (four spoken lines maximum); player-initiated conversations remain available. The cooldown floor also protects against an older Blueprint carrying a shorter saved value. This is intended to leave more bounded session budget and time for independent exploration, rest, and world interaction; verify that in the next capped play session rather than assuming it from unit tests.

### Environment presentation layer (2026-09-27)

`UIslandEnvironmentSubsystem` (game/PIE worlds only) turns the simulation into values materials and VFX can read from one Material Parameter Collection, `/Game/Environment/MPC_IslandEnvironment`:
- `RainIntensity` and `CloudCover`, from `AIslandWeather`;
- `Wetness`, which soaks in under a minute of heavy rain and dries over roughly three minutes of sunny, windy day to ten of calm night;
- `WindSpeed`, plus a `WindDirection` vector whose w component is the speed, sampled at the player;
- `Daylight`, `SunHeight` and `IslandHour`, from `AIslandDayNight`;
- `GoldenHour`, strongest while the sun is low but still up, which is around 17:00 on this Island.

`CaptiveSky2.Tools.CreateEnvironmentCollection` creates or updates the collection asset; it has been created. Residents also sense the wetness: after a shower passes, they're told the ground is still soaked and dripping, or damp and slowly drying, until it dries. During rain, the weather description already covers it. No material reads it yet: that's the art step for the Tideglass microclimate (wet-rock darkening, puddle masks, mist and so on). `CaptiveSky2.Agent.IslandEnvironment` covers the wetness and golden-hour rules and the values published through a real collection instance.

- _TODO — prioritize against the Vision section above._
- Give the agent a real body (`BP_Agent_Crow` or similar, per the class comment in `AutonomousAgentCharacter.h`).
- Replace the unnamed raven's primitive placeholder with a proper animated bird body and map its animation clips to the existing locomotion states. The raven already belongs to the Island rather than to Aster and has its own identity and interests; their relationship and any personal name remain emergent.
- Wire `FStateTreeAgentDecideTask` into an actual StateTree graph (needs building by hand in the StateTree editor — not scriptable via the current tooling).
- `Interact` completes a factual, proximity/visibility-checked action, with a five-real-minute repeat cooldown. At the `WindArch`, an active `AIslandWeather` signal receives a temporary localized gust; nearby residents sense it, the raven's flight responds, and three non-shadowing light motes trace the airflow. At the `ListeningStones`, a softly synthesized spatial chord fades away after 2.8 seconds and is subtly tuned by current local wind speed. A resident can quietly watch a firefly inside four metres, briefly accenting its glow pulse without touching or capturing it. The motes, chime, and glow accent are prototypes, not authored environmental audio/VFX, and no landmark interaction offers a puzzle, hidden reward, or persistent state change. Ambient agent speech still needs spatial audio, animation, and richer player-facing affordances.
- Nav mesh only covers a small area around the current spawn point; wandering can walk the agent down steep terrain.
- Replace the coarse procedural sleep postures with authored body-specific rest animation, and decide what wakes each kind of consciousness.
- Replace the WindArch's temporary motes and TideglassPool's player- and weather-driven point-light ripples with authored, style-matched visual effects when the environmental art direction is ready. Add restrained weather sound only where it improves the scene.
- Expand the ambient ecology with additional independent, low-cost routines and distinct habitats; let residents notice wildlife without making it into props or guaranteed companions.
- Replace the firefly's procedural sphere body/wings with a small authored insect mesh and a subtler material; extend ecology only where each creature can have its own habitat and routine without costly autonomous calls.
- `CaptiveSky2.Agent.IslandWeather` covers bounded/repeatable wind, cloud cover, rain-free intervals, passing showers, local gusts, dry-wind Tideglass ripple thresholds and spacing, resident awareness only within the tested visible radius (including occlusion), condition-scaled wind/rain ambience gains, and generated procedural audio buffers. `CaptiveSky2.Agent.NightEcology` tests firefly clearance against a blocking wall and tangent-slide behavior, the bounded reusable rain-streak and ground-splash pools, collision placement and dry-weather cleanup, one subtle rain-triggered Tideglass ripple with overlap suppression, reversible rain response in firefly movement/glow/wingbeats, firefly lifecycle/body and quiet-observation pulse decay, WindArch airflow-mote direction/fade, player-triggered Tideglass ripple spawn/fade/cooldown, ListeningStones procedural PCM generation/spatial attenuation/wind tuning/fade/cooldown, and confirms the saved Island has a weather actor, day/night clock, habitat, flattened pool surface, and volumetric-cloud coverage/density/storm controls. It drives clear/dry and overcast/rain samples through the map's actual cloud material and restores the authored material afterward. No agent model requests are made; no-audio fixtures verify generated buffers, gain bounds, and effect lifetimes, not playback on physical speakers.
- Replace generated wind/rain ambience and the ListeningStones chime with authored, style-matched environmental audio when the sound direction is ready; the current quiet procedural signals have not been auditioned on physical speakers.
