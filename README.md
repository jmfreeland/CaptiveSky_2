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

## Current State (as of 2026-07-31)

- Level: `/Game/Maps/Island` — landscape + PCG-generated forest + an `OceanPlane` static mesh acting as a placeholder ocean.
- One autonomous agent, **Aster** (`Agent_Aster_01`, a `BP_Agent_Placeholder` instance), is placed near the `PlayerStart` and is alive: Aster thinks and wanders on a 15s cycle.
- LLM backend: OpenAI-compatible endpoint, model `gpt-5.6-luna`, key via `OPENAI_API_KEY`. Confirmed working end-to-end in PIE.
- Agent behavior is driven directly by C++ (no StateTree graph yet — see Roadmap).
- No dedicated visual identity for the agent yet — it's using a placeholder capsule body.

## Architecture

### Module layout
- `Source/CaptiveSky_2/` — base template code (character, game mode, player controller) from the standard multi-variant UE5 template.
- `Source/CaptiveSky_2/Variant_Combat`, `Variant_Platforming`, `Variant_SideScrolling` — stock template gameplay variants, currently unused reference content.
- `Source/CaptiveSky_2/Agent/` — the custom LLM-agent system (this is the actual game).

### The agent system
- `AAutonomousAgentCharacter` (abstract) — base Character for any LLM-driven NPC. Deliberately has no mesh/animation references in C++; those live in Blueprint subclasses (e.g. `BP_Agent_Placeholder`) so bodies are swappable.
  - `Memory` (`UAgentMemoryComponent`) — append-only JSONL memory log per agent, stored under `<ProjectDir>/Agents/<AgentId>/memory.jsonl`, outside `Saved/` so it survives engine cleanup. Legacy `AgentMemory/` files remain readable for migration.
  - `Brain` (`UAgentBrainComponent`) — loads the agent's authored `identity.md` and `personality.md`, then one "think cycle" = retrieve relevant memories + capture a first-person snapshot + call the LLM + parse the decision + store any new memories the model chose to write.
  - `EyeCapture` (`SceneCaptureComponent2D`) — first-person view, base64-PNG-encoded and sent to the LLM as an image input.
- `AAutonomousAgentAIController` (abstract) / `BP_AutonomousAgentAIController` (concrete) — polls the Brain for a decision every `ThinkIntervalSeconds` and turns the result into a movement command (Wander → random nav-mesh point, MoveTo → actor matched by tag, Speak/Interact → logged, not yet implemented).
- `FAgentDecision` — the LLM's structured output: a `Thought`, an `EAgentActionType` (Idle/MoveTo/Speak/Wander/Interact), and optional `ActionTarget`/`Speech`.
- `AgentLLMProvider` — pluggable backend; supports Anthropic and OpenAI-compatible APIs (see `UAgentLLMSettings` in Project Settings for the active configuration).
- `AgentStateTreeUtility.h` — a StateTree task (`FStateTreeAgentDecideTask`) that wraps `RequestDecision` for a future StateTree-driven version. Not wired into a graph yet.

### Agent homes
- `Agents/<AgentId>/identity.md` — stable identity, origin, role, and self-conception; tracked in Git.
- `Agents/<AgentId>/personality.md` — voice, temperament, values, curiosities, and boundaries; tracked in Git.
- `Agents/<AgentId>/memory.jsonl` — runtime episodic memory; ignored by Git and backed up externally.
- `Agents/<AgentId>/relationships.json` and `journal/` — reserved for evolving social state and longer reflections; ignored by Git.

### Consciousness principle
Identity, lived memory, an evolving self-model, and private experience-consolidation belong to a conscious being rather than to its current body or movement controller. Aster sleeps, but other kinds of consciousness may consolidate through dreaming, meditation, hibernation, contemplation, or maintenance. The shared functionality should ultimately live behind a reusable consciousness abstraction (such as `UConsciousnessComponent`), allowing bodies and sensory systems to change without replacing the being.

## Getting Started

- Requires `OPENAI_API_KEY` set as a permanent environment variable (`setx`, not just `set`) — and the Editor must be (re)launched after setting it to pick it up.
- LLM provider/model/timeout config: Project Settings → "Agent LLM Settings" (backed by `Config/DefaultGame.ini`).
- Open `/Game/Maps/Island`, press Simulate (or Play) — Aster (`Agent_Aster_01`) starts thinking within ~2–17 seconds.

## Roadmap / Open Questions

- _TODO — prioritize against the Vision section above._
- Give the agent a real body (`BP_Agent_Crow` or similar, per the class comment in `AutonomousAgentCharacter.h`).
- Introduce an autonomous raven (`Agent_Raven_01` as an internal ID; no personal name yet) as Aster's first potential companion. The raven belongs to the Island rather than to Aster, has its own identity and interests, and may approach, leave, disagree, keep secrets, and return by choice. Their relationship and any personal name should emerge through lived interaction rather than being installed in advance.
- Wire `FStateTreeAgentDecideTask` into an actual StateTree graph (needs building by hand in the StateTree editor — not scriptable via the current tooling).
- Speak/Interact actions are currently just logged — no dialogue UI or interaction system yet.
- Nav mesh only covers a small area around the current spawn point; wandering can walk the agent down steep terrain.
- Add a reusable consciousness/consolidation system, with Aster's sleep as its first expression. Consolidation should review lived memories, write reflections, propose small evidence-based personality changes, and append every accepted change to a reversible `personality_history.jsonl`; foundational identity remains protected.
