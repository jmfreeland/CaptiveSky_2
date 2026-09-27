# Temporal Simulation Architecture

> **Time can be compressed. Causality can't be.**

## Status

Design direction / architecture proposal. This document deliberately separates near-term implementable pieces from longer-term research problems.

## North star

Captive Sky is a canonical persistent world unto itself. Unreal Engine is its premier high-fidelity lens, not the container in which reality exists.

The same resident, object, project, relationship, place, or historical fact must remain coherent whether it is currently represented by:

- high-frequency Unreal simulation,
- minute/hour-scale activity simulation,
- day/month-scale life simulation,
- year/decade-scale historical simulation, or
- no graphical client at all.

Changing temporal resolution may change **how** an outcome is computed. It must not silently change **what kind of world** Captive Sky is.

Observation should not determine reality. Causal significance should determine simulation depth.

## 1. Hierarchical temporal resolution

These are conceptual bands, not necessarily four separate engines.

| Resolution | Typical interval | Primary concerns |
| --- | --- | --- |
| Embodied | milliseconds–seconds | movement, gaze, speech timing, animation, physics, immediate perception |
| Activity | minutes–hours | travel, work, cooking, conversations, making, maintenance |
| Life | days–months | relationships, projects, learning, habits, reputation, construction |
| Historical | months–decades | settlements, institutions, ecology, culture, infrastructure, architecture |

Higher levels provide intentions, constraints and schedules to lower levels. Lower levels emit durable consequences that roll upward.

Example: “build a workshop” can exist simultaneously as a long-running project, today's work session, and—in an observed Unreal scene—individual embodied construction actions. These are representations of one causal process, not separate realities.

## 2. Canonical state + events

Captive Sky should maintain canonical world state plus an appendable history of meaningful events.

Illustrative events:

- resident begins an activity;
- resident travels from A to B;
- conversation changes a relationship;
- skill is practiced or taught;
- object is created, transferred, damaged or destroyed;
- construction project advances;
- vegetation is planted or removed;
- weather interrupts an activity;
- cultural work is observed, imitated or preserved.

The event is canonical regardless of whether it was produced by detailed Unreal interaction, deterministic simulation, or LLM-assisted reasoning.

Events should carry stable entity IDs, world time, causes/parents where useful, location/context, state deltas and enough provenance to explain important outcomes.

## 3. Promotion and demotion between resolutions

### Promotion: coarse → fine

When a location/entity becomes causally important or an observer arrives, the system materializes a finer representation consistent with canonical state.

If a workshop is 63% complete, Unreal should instantiate geometry corresponding to that stage, consumed materials should remain consumed, current tasks should make sense, nearby world modifications should be present, and residents should possess the memories/relationships implied by prior events.

Promotion must not invent contradictory history merely to make a scene look busy.

### Demotion: fine → coarse

When detailed simulation is no longer needed, significant fine-grained consequences collapse into durable state/events.

If the player watches a resident break a unique bowl, leaving the area cannot resurrect it. If a detailed conversation materially changes a relationship or creates a promise, that consequence survives after Unreal stops simulating the encounter.

Ephemeral detail may be discarded. Causally meaningful detail may not.

## 4. World invariants

Across every temporal resolution:

1. **Identity is conserved.** Stable entities do not silently become different entities.
2. **Causal ordering is conserved.** Effects cannot precede their required causes.
3. **Ownership and custody are conserved** except through explicit transfer/loss processes.
4. **Resources are accounted for** unless a defined process creates, transforms or destroys them.
5. **Important physical modifications persist.**
6. **Relationships change through defined causes**, not arbitrary coarse-resolution rewrites.
7. **Important memories and historical facts are not silently rewritten.**
8. **Projects have prerequisites, inputs, effort and consequences.**
9. **World time is canonical**, even if different subsystems advance through it at different granularities.
10. **Rendering is non-authoritative.** Destroying an Unreal Actor must not imply destroying the underlying entity.

These invariants should become executable tests where practical.

## 5. Event-driven advancement

When no client requires real-time simulation, the world should not necessarily step every entity through uniform tiny dt intervals.

Prefer scheduling the next meaningful transition:

`advance_to_next_event()`

Examples:

- sleep until expected wake time;
- journey reaches waypoint;
- weather front arrives;
- project work session begins;
- two residents' schedules intersect;
- resource becomes unavailable;
- construction stage completes.

An event can trigger finer simulation when uncertainty or interaction becomes causally important.

This allows long periods of routine life to advance cheaply while still spending computation on consequential moments.

## 6. Adaptive fidelity

Simulation fidelity should depend on causal significance as well as observation.

A resident baking alone may initially be represented as an activity with expected inputs, duration and outcome. If another resident arrives, an unusual failure occurs, or the activity intersects with an important goal, the system can promote the episode into deeper agent/LLM reasoning even with no human observer.

Conversely, a human camera pointed at an uneventful distant process does not necessarily require expensive cognitive simulation; Unreal may need visual fidelity while world reasoning remains simple.

This separates **render fidelity**, **physical fidelity**, and **cognitive/social fidelity**.

## 7. LLM responsibilities

LLMs should primarily provide interpretation, intention, planning, language, creative choices and reasoning under uncertainty.

They should not directly rewrite canonical reality.

Bad:

> Three months pass; Rowan becomes a famous sculptor and builds a gallery.

Better:

> Rowan develops an interest in sculpture and forms an intention to practice.

The world then determines whether Rowan has time, materials, teachers, opportunities and persistence. Skill changes through actual practice/events. Reputation emerges from other residents observing and communicating about work. A gallery requires a realizable project.

LLMs propose meaningful action; deterministic/stateful systems validate and apply world consequences.

## 8. Spatial and architectural history

Resident-driven world modification should use persistent operations layered above largely immutable base geology:

- paths and roads;
- vegetation clearing/planting;
- grading represented safely where possible;
- foundations;
- retaining walls and embankments;
- excavation;
- utilities;
- structures;
- public spaces;
- cultural artifacts.

Prefer recording operations and provenance over destructively baking an opaque final level.

A road should be able to answer, in data, why it exists: who initiated it, what need produced it, when it was improved, and which later structures depended upon it.

This permits individual decisions → paths → clusters → infrastructure → neighborhoods → architecture → cities.

## 9. The difficult problem: visual materialization of accelerated history

World-state evolution is easier than producing convincing, high-quality visual evidence for every possible evolved state.

This is a first-class research problem, not something to hand-wave away.

### Likely tractable early

- choosing among authored modular structures;
- spline paths/roads;
- PCG vegetation suppression and regrowth;
- construction-stage variants;
- material parameter changes for wetness, dirt, age and wear;
- prop accumulation;
- deterministic dressing from seeds;
- predefined foundation/retaining-wall systems;
- procedural placement within strongly constrained grammars.

### Moderately difficult

- buildings that expand organically while remaining architecturally convincing;
- coherent infrastructure networks;
- resident-created gardens/public spaces;
- believable repair, damage and renovation;
- neighborhoods acquiring distinct visual styles through cultural imitation;
- visual aging over decades without storing bespoke versions of everything.

### Hard / research-heavy

- arbitrary resident-designed architecture that is simultaneously beautiful, structurally plausible, navigable and performant;
- runtime generation of production-quality meshes/materials for genuinely novel objects;
- large-scale terrain deformation that remains compatible with navigation, PCG, water and persistence;
- rapid century-scale city evolution without visual discontinuities;
- generating enough high-quality asset diversity that emergent culture does not visibly reduce to combinations of a small kit.

### Practical strategy

Do not require arbitrary generative geometry for the first version.

Represent **intent and history generatively**, but initially realize them through excellent constrained visual grammars: modular kits, parametric assemblies, PCG rules, splines, material variation, staged construction and carefully authored asset families.

This preserves emergence while keeping the visible world art-directable.

Over time, Tripo or other asset-generation pipelines may expand the vocabulary, but generated assets should pass validation/optimization before becoming canonical world content.

## 10. Visual provenance

Visible objects should ideally know both **what they are** and **why this representation was selected**.

For example:

- canonical entity: Rowan's workshop;
- function: woodworking workspace;
- footprint/site constraints;
- builder/cultural influences;
- construction date and modifications;
- current condition;
- visual grammar/style parameters;
- selected modular components;
- deterministic generation seed.

This lets Unreal rebuild the same recognizable place after unloading it, while allowing controlled evolution when canonical history changes.

## 11. Testing strategy

Build headless simulation tests early.

Useful tests:

- run equivalent initial states at multiple temporal resolutions and compare invariant violations;
- advance 30/365/3650 world days without Unreal rendering;
- repeatedly promote/demote the same residents and verify conservation;
- verify unique objects cannot duplicate or resurrect;
- verify resources and construction inputs balance;
- verify deterministic materialization from the same canonical state/seed;
- stress-test thousands of scheduled activities;
- compare statistical behavior across coarse and fine simulations without demanding identical histories;
- replay event histories into a clean state and compare resulting canonical state.

A particularly useful harness would run the same scenario under fine and coarse resolution and report divergences in causal structure.

## 12. Near-term implementation path

1. Define stable world/entity IDs and clarify which existing Unreal state is canonical versus presentation-only.
2. Define a small event schema and append-only event log around existing persistence.
3. Choose one activity—travel or a simple project—and implement coarse advancement plus fine materialization.
4. Add promotion/demotion tests.
5. Build one persistent construction object with staged visual states.
6. Make PCG/visual presentation respond deterministically to that object's canonical state.
7. Only then broaden toward longer offline advancement and cloud-hosted WorldState.

## 13. Open questions

- What exact state belongs in canonical WorldState versus derivable caches?
- Which events require LLM reasoning and which should remain deterministic?
- How should uncertainty be represented before an event is resolved?
- How much event history is retained verbatim versus summarized/compacted?
- Can old history be compressed without losing provenance needed by residents?
- How do simultaneous events and conflicts resolve?
- How do we preserve reproducibility while still allowing model-driven creativity?
- What is the contract between canonical spatial state and Unreal transforms?
- How should generated assets be validated, versioned and persisted?
- What visual changes can safely be parametric, and which require authored/generated geometry?
- How do we prevent accelerated evolution from outrunning our ability to materialize it beautifully?

## Design principles

> **Time can be compressed. Causality can't be.**

> **Observation should not determine reality; causal significance should determine simulation depth.**

> **Nothing exists merely because it is being rendered, and nothing stops existing merely because we stop rendering it.**

> **Unreal doesn't contain Captive Sky. Unreal lets you visit it.**

> **Simulation should leave beautiful evidence behind.**
