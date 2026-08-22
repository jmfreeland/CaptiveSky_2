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

## Current State (as of 2026-08-10)

- Level: `/Game/Maps/Island` — landscape + PCG-generated forest + an `OceanPlane` static mesh acting as a placeholder ocean.
- Two autonomous agents are present: **Aster** (`Agent_Aster_01`) and an intentionally unnamed raven (`Agent_Raven_01`). Each has an independent identity, personality, memory, relationships, and consciousness lifecycle.
- LLM backend: OpenAI-compatible endpoint, model `gpt-5.6-luna`, key via `OPENAI_API_KEY`. Confirmed working end-to-end in PIE.
- Agent behavior is driven directly by C++ (no StateTree graph yet — see Roadmap).
- No dedicated visual identity for the agent yet — it's using a placeholder capsule body.
- A source-controlled conversation UI lets the player speak to a nearby autonomous agent; both sides of the exchange are stored as lived conversation memory.
- The raven lives near Aster in a temporary primitive body and has a body-agnostic prototype flight controller; a proper animated bird asset is still needed.
- A standalone, multi-agent external gateway now provides a channel-neutral correspondence boundary, with Discord as its first adapter. It can host the raven headlessly from the same identity, personality, and JSONL memory when Unreal is offline; Discord activation still requires a private bot token.
- Nearby agents can initiate bounded, reciprocal conversations. Speech appears as ambient subtitles, and both participants retain neutral factual relationship evidence; familiarity measures exposure only, never assumed trust or affection.
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
  - `EyeCapture` (`SceneCaptureComponent2D`) — first-person view, base64-PNG-encoded and sent to the LLM as an image input.
- `AAutonomousAgentAIController` (abstract) / `BP_AutonomousAgentAIController` (concrete) — polls the Brain for a decision every `ThinkIntervalSeconds` and turns the result into movement. Nearby targeted speech now enters the social layer; `Interact` remains a future action.
- `UAgentExternalBridgeComponent` — inherited by every autonomous body; publishes a short-lived embodiment lease, consumes durable channel-neutral turns one at a time, and returns embodied speech through the gateway outbox.
- `FAgentDecision` — the LLM's structured output: a `Thought`, an `EAgentActionType` (Idle/MoveTo/Speak/Wander/Interact), and optional `ActionTarget`/`Speech`.
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

- _TODO — prioritize against the Vision section above._
- Give the agent a real body (`BP_Agent_Crow` or similar, per the class comment in `AutonomousAgentCharacter.h`).
- Replace the unnamed raven's primitive placeholder with a proper animated bird body and map its animation clips to the existing locomotion states. The raven already belongs to the Island rather than to Aster and has its own identity and interests; their relationship and any personal name remain emergent.
- Wire `FStateTreeAgentDecideTask` into an actual StateTree graph (needs building by hand in the StateTree editor — not scriptable via the current tooling).
- `Interact` actions are currently just logged. Ambient agent speech has subtitle presentation but still needs spatial audio, animation, and richer player-facing affordances.
- Nav mesh only covers a small area around the current spawn point; wandering can walk the agent down steep terrain.
- Give sleep a physical expression per body (Aster settling somewhere safe, the raven roosting) and decide what wakes each kind of consciousness.
