# CaptiveSky

- Captive Sky is an interactive world. Consciousness in some form is meant to infuse almost everything, and it's a place to experiment with interactive creation. Ideally it should be multiplayer long term and allow concurrent access to different levels via 'elevator' functionality in each map. There will be more autonomous agents than humans, and they will have memory as well as some ability to shape the environment itself. Music is important in this world as are art, mathematics, and humor. 

## Vision

_TODO — fill in. Some prompts to dig into:_

- **Premise / theme.** Captive Sky is just a play on an enclosed world like a snow globe. 
- **Genre & core loop.** For now, its about exploring, interacting, communicating (human/nonhuman), and creating. 
- **Why the LLM-driven agents matter.** They're here to build the world along with anyone involved. It's for them as much as anyone. 
- **Setting.** Myriad worlds over time.
- **Target feel.** Snow Crash, Ready Player One, Bobiverse, Terry Pratchett, Dune, Scalzi, Robert Jordan, etc. 

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
- Nearby agents can initiate bounded, reciprocal conversations or choose to approach one another first using stable, body-independent movement targets. Approaching never forces a conversation. Speech appears as ambient subtitles, and both participants retain neutral factual relationship evidence; familiarity measures exposure only, never assumed trust or affection.
- When `AIslandWeather`, `AIslandDayNight`, and the `TideglassPool` habitat are present, three independent firefly prototypes drift there at night and disappear at dawn. Their individual paths and blinks vary, local wind gently nudges them, and a segmented sphere-based body with beating placeholder wings gives the light a more insect-like silhouette. Nearby residents can notice them as wildlife; they do not make model calls, follow residents, or persist as owned companions. Proper authored insect art, a subtler material, and sound remain future work.
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

The first implementation adds an optional `AIslandWeather` actor: repeatable slowly changing cloud-cover and spatial wind signals, geometry-based upwind shelter, embodied weather observations for all agents, and drift during raven cruising flight. An independent slow rain-front signal creates occasional bounded showers only when cloud cover is high. The existing authored volumetric cloud material gets a transient dynamic instance while weather is active; its `Cloud_GlobalCoverage`, `Cloud_GlobalDensity`, and `StormClouds` parameters follow the cloud/rain signals gently, while cloud cover also dims the sun and skylight (maximum reductions of 24% and 30%). Rain drives a finite pool of at most 192 translucent instanced streaks around the player, slanted by sampled local wind and hidden when the front fades; this avoids per-drop actors and extra agent/model work. During stronger showers, TideglassPool also gets a brief, dim water ripple no more often than every 4.5 seconds, reusing its existing finite effect. Nearby residents are told this is an independent weather response, not a discovery or a consequence of their action. Residents receive the current simulated rain intensity. The authored level material is restored when the weather actor ends. Wind/rain audio and ground impacts remain future work. Place one weather actor per level; without it, existing lighting, cloud material, and flight are unchanged. Weather time currently restarts with each play session.

The Island contains one weather actor and two roost candidates, `Roost_West` and `Roost_East`, near the original raven spawn. The original six white ledge/back/roof blocks have been replaced by a rocky west perch and a spruce-side east fork, with satellite stones, smaller trees, ground plants and fallen wood. The rock and spruce were copied from the original project's StarterContent and PN_interactiveSpruceForest assets; the fork and fallen wood still use simple wood-textured cylinders pending authored branch meshes. Tree foliage is nonblocking, with a separate solid trunk and branch support. Shelter remains dependent on wind direction and actual solid geometry, not the site's name.

The raven controller now keeps its capsule upright, clears residual CharacterMovement velocity, ignores itself in ground traces, and stages roost travel through ascent, overhead approach and descent. It only enters `Perched` after finding nearby upward-facing support; obstructions abort the route. This is still a simple approach, not general flight pathfinding around obstacles. `RequestPerch(Tag)` is available to Blueprint. `CaptiveSky2.Agent.RavenPerch` covers arrival/departure and blocked/unsupported targets in a brain-free fixture; when Island is the open editor map, it also checks both placed roosts against real scene collision without creating agent memories.

### Day and night (2026-09-13)

`AIslandDayNight` provides one shared local clock per level. The Island's `Island_DayNight` actor references the existing DirectionalLight and SkyLight and owns a second atmosphere light for the moon. Defaults are a 40-minute full day, beginning at 09:00; sunlight moves across the sky, warms near dawn/dusk and fades below the horizon, while cool moonlight and reduced skylight illuminate the night. The existing atmosphere and volumetric clouds respond to the moving lights. This does not yet add stars, lunar phases, seasons, or lightning.

Adjust **Start Hour**, **Day Length Minutes**, **Day Sun Intensity**, and **Moon Intensity** on the actor; **Advance Time** pauses the clock. Editing Start Hour previews lighting outside play. During play, all embodied agents receive the current phase and approximate Island time in their observations. When an `AIslandWeather` actor is present, its normalized cloud-cover signal changes the existing volumetric cloud coverage/density, a separate rain-front signal adds a restrained storm-cloud layer when overcast, and cloud cover softly dims direct sunlight/skylight; without it, authored clouds and original clear-weather lighting are preserved. The clock advances with simulation time and resets on a new play session; it is not yet persisted, multiplayer-replicated, or tied to real-world time. Settled agents can rest at night; see the session safeguards below. `CaptiveSky2.Agent.DayNight` checks clock wrapping, the sun's daily arc, and cloud-illumination bounds.

Three additional whitebox points of interest are now placed near the spawn: `ListeningStones`, `TideglassPool`, and `WindArch`. Their center markers carry `IslandLandmark` and `RavenInterest` tags, so nearby agents can perceive and approach them without being told what they are. They remain simple prototypes awaiting authored art. Interacting with the ListeningStones plays a quiet, locally synthesized, spatially attenuated chord that fades after 2.8 seconds; it uses no external audio asset and leaves no persistent change. Interacting with the WindArch creates a local simulated gust and three small illuminated motes tracing its airflow; both fade after eighteen seconds of Island time and leave no persistent change. At the TideglassPool, `Interact` produces a transient ring of moving cool highlights across the flattened sphere prototype; it fades in about 1.6 seconds and leaves no persistent change. The pool also serves as the habitat anchor for a small, night-only prototype firefly population; agents may quietly watch a firefly within four metres to elicit a brief glow accent, but it remains wild and uncapturable.

### Landscape material (2026-09-13)

The Island uses `/Game/Materials/MI_Island_Landscape`, adapted from the original `CaptiveSky` project's `/Game/Materals/MI_MountainRange`. Its mossy grass, rocky ground and cliff textures replace the flat placeholder. The original source project is unchanged, and `/Game/Materials/M_Island_Auto` remains available to restore the previous look.

The new local parent `/Game/Materials/M_Island_Textured_Auto` connects the source graph's automatic slope blend directly to Material Attributes because this Island has no painted landscape layers. Painted Path/Road layers are therefore not active in this variant. Snow, puddles, ground displacement and the old RVT preview switch are disabled on the instance for this first pass. Existing landscape shape and collision are unchanged.

`Scripts/Import-LandscapeMaterial.ps1` provides a conservative dependency preflight and optional `-Copy` from the original project. The first import copied 82 assets (about 837 MiB), including the source material's foliage and RVT dependencies; copying those dependencies alone does not configure foliage placement or RVT volumes. After copying, validate package references in Unreal. Content and the saved map remain outside Git and use the existing external backup workflow.

Nesting candidates use TargetPoint actors positioned at the raven's capsule centre when perched, with a unique movement tag **first**, then `RavenPerch` and `RavenNestSite`. Within 25 metres and unobstructed line of sight, the raven can perceive up to four candidates, each with a current horizontal upwind-obstruction assessment, and deliberately `move_to` a candidate and perch. In rough weather it may consider a visible roost before resting, but this remains its decision rather than an automatic or forced route; bird-sized perches are not offered as Aster's movement targets. The assessment uses the same six-metre visibility trace that attenuates local wind; it explicitly does not claim overhead rain cover or safe support. A marker alone does not create shelter: place it over a solid ledge/branch with nearby protective geometry. No home is assigned, no nest-building or ownership is simulated yet, and roost choice does not force or change sleep. `CaptiveSky2.Agent.IslandWeather` checks the trace against a real blocking fixture as well as bounded, repeatable, spatially and temporally varying wind and normalized cloud cover.

- _TODO — prioritize against the Vision section above._
- Give the agent a real body (`BP_Agent_Crow` or similar, per the class comment in `AutonomousAgentCharacter.h`).
- Replace the unnamed raven's primitive placeholder with a proper animated bird body and map its animation clips to the existing locomotion states. The raven already belongs to the Island rather than to Aster and has its own identity and interests; their relationship and any personal name remain emergent.
- Wire `FStateTreeAgentDecideTask` into an actual StateTree graph (needs building by hand in the StateTree editor — not scriptable via the current tooling).
- `Interact` completes a factual, proximity/visibility-checked action, with a five-real-minute repeat cooldown. At the `WindArch`, an active `AIslandWeather` signal receives a temporary localized gust; nearby residents sense it, the raven's flight responds, and three non-shadowing light motes trace the airflow. At the `ListeningStones`, a softly synthesized spatial chord fades away after 2.8 seconds. A resident can quietly watch a firefly inside four metres, briefly accenting its glow pulse without touching or capturing it. The motes, chime, and glow accent are prototypes, not authored environmental audio/VFX, and no landmark interaction offers a puzzle, hidden reward, or persistent state change. Ambient agent speech still needs spatial audio, animation, and richer player-facing affordances.
- Nav mesh only covers a small area around the current spawn point; wandering can walk the agent down steep terrain.
- Replace the coarse procedural sleep postures with authored body-specific rest animation, and decide what wakes each kind of consciousness.
- Replace the WindArch's temporary motes and TideglassPool's player-triggered point-light ripple with authored, style-matched visual effects when the environmental art direction is ready; rain uses a quieter variant of that existing pool effect. Add restrained weather sound only where it improves the scene.
- Expand the ambient ecology with additional independent, low-cost routines and distinct habitats; let residents notice wildlife without making it into props or guaranteed companions.
- Replace the firefly's procedural sphere body/wings with a small authored insect mesh and a subtler material; extend ecology only where each creature can have its own habitat and routine without costly autonomous calls.
- `CaptiveSky2.Agent.IslandWeather` covers bounded/repeatable wind, cloud cover, rain-free intervals, passing showers, and local gusts. `CaptiveSky2.Agent.NightEcology` tests the bounded reusable rain-streak pool (translucent material, visible during a front and hidden afterward), one subtle rain-triggered Tideglass ripple with overlap suppression, firefly lifecycle/body and quiet-observation pulse decay, WindArch airflow-mote direction/fade, player-triggered Tideglass ripple spawn/fade/cooldown, ListeningStones procedural PCM generation/spatial attenuation/fade/cooldown, and confirms the saved Island has a weather actor, day/night clock, habitat, flattened pool surface, and volumetric-cloud coverage/density/storm controls. It drives clear/dry and overcast/rain samples through the map's actual cloud material and restores the authored material afterward. No agent model requests are made; the no-audio fixture verifies generated signal/lifetime, not playback on physical speakers.
- Add restrained rain impacts and wind/rain audio; precipitation now has visible pooled streaks, but no ground/water response or sound.
