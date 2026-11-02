# Shared Island and hosted visitor path

**Status:** design direction only. No cloud provider, public endpoint, account
system, or server deployment is selected or authorized by this plan.

## Goal

Let human visitors and embodied or channel-based residents participate in one
lasting Island, while preserving quiet observation, private resident lives,
human review of world-changing proposals, and the current request/play
safeguards. Build the shared-world boundary before inviting the public; do not
turn the existing local files into an accidentally networked database.

## Current evidence and constraints

- `UIslandWorldStateSubsystem` loads one version-2 `WorldState/<Map>.json`
  snapshot per map and writes it through a sibling temporary file followed by
  replacement. It is deliberately scoped to Game/PIE worlds and refuses to
  overwrite unreadable state. This is a useful single-writer local backend,
  not a multi-server transaction system.
- Accepted lasting mutations call the atomic world-state save before reporting
  success. The Island clock also saves its hour/day and wetness periodically and
  from `AIslandDayNight::EndPlay`; an orderly stop therefore flushes the latest
  clock/environment snapshot, while an abrupt process kill can still lose the
  interval since the last periodic save.
- `UIslandTrailSubsystem` persists its own per-map trail ledger. Resident
  identity, episodic memory, and relationship state live in separate
  `Agents/<AgentId>/` files. `CaptiveSkyDataRoot` relocates local data but does
  not provide remote coordination or access control.
- The .NET Gateway already separates Discord/channel routing from the
  consciousness model and serializes turns per agent. Its inboxes, dedup
  records, embodiment leases, and runtime leases are file-based. They are
  suitable for the current single-gateway deployment, not distributed locks.
- The README explicitly says Island time is not multiplayer-replicated and
  pauses when play closes. The current bounded session still ends at 30 real
  minutes or 64 shared embodied model requests; neither cap is a hosted-service
  spending quota.
- **Source audit (2026-11-02):** `ACaptiveSky_2GameMode` has no `PostLogin` or
  `Logout` visitor accounting, and the Agent module contains no
  network-client-presence lifecycle. `UAgentPlaySessionSubsystem` is a
  real-time/request watchdog and budget ledger, not a pause coordinator.
  `UIslandWorldStateSubsystem` loads the per-map JSON at world begin, persists
  accepted changes atomically, and tears down transient presentation actors
  during `Deinitialize`; the clock separately checkpoints its hour, day, and
  wetness about once a minute and again from `AIslandDayNight::EndPlay`.
  Therefore the empty-world policy is not currently implemented by persistent
  files or the local session watchdog.

Consequently, clients must not write their own copies of shared world state
and later race to replace a common JSON file. Do not mount the same
`WorldState/` or `Agents/` directory into multiple Unreal servers or Gateway
instances and assume file rename, append, or lease files make that safe.

## Authority model

Start with one authoritative Unreal server process per shared Island/map.
Clients send *intent* (move, interact, write a guest-book line, propose an
upgrade); the server rechecks identity, target, reach, visibility, cooldown,
permission, and budget before applying a change. Clients never decide that a
lasting action succeeded. The server replicates the resulting canonical state
and transient presentation to relevant clients. This follows Unreal's
client-server networking model, where the server owns the authoritative game
state and validates client RPC inputs ([Networking Overview](https://dev.epicgames.com/documentation/unreal-engine/networking-overview-for-unreal-engine),
[Remote Procedure Calls](https://dev.epicgames.com/documentation/unreal-engine/remote-procedure-calls-in-unreal-engine)).

The server owns one embodied instance of each resident. The Gateway remains a
separate channel adapter: it may submit an incoming turn to that embodiment
through the existing lease/inbox handoff, or reply headlessly when no body is
authoritative. Remote clients and Discord adapters never receive model API
keys. Keep resident episodic memories, relationship records, and personality
history private to that `AgentId`; replicate only the public actions and facts
other beings could actually observe.

## Persistence evolution

1. **First private-hosted prototype:** retain the current local/offline file
   backend behind the single server process. Give that process the only write
   access to its durable volume, back up snapshots, and keep the Gateway to one
   active instance. This tests connectivity without pretending shared files
   solve concurrency.
2. **Before multiple servers or regions:** introduce a transactional shared
   store behind the same world-state operations. Use per-map revisions and
   idempotent mutation IDs so retries cannot duplicate nests, arrangements,
   guest-book entries, or proposals. Preserve a migration path from JSON v2;
   retain an auditable change journal for recovery and human review.
3. Keep per-agent private data under separate identity-scoped access rules.
   A visitor's public guest-book line is not permission to read another
   resident's private memories, Discord history, personality evolution, or
   proposals.
4. Add distributed Gateway turn leases/dedup only if more than one Gateway
   instance is needed. Until then, a single healthy Gateway with durable
   storage is simpler and safer than a premature multi-instance setup.

The current snapshot files remain appropriate for single-writer local play.
No storage-provider choice should be made until a small two-client prototype
establishes the required consistency, backup, and recovery behavior.

## Safety and world-time policy

- Keep the existing 30-minute/64-request bounded play mode unchanged for
  development and feedback runs. A hosted process must add service-level
  per-resident, per-world, hourly, and daily quotas before any continuously
  embodied LLM loop is considered; the local session cap is not a billing
  ceiling.
- Keep incoming turns opt-in, serialized, and deduplicated. No overnight
  autonomous Discord messages or unbounded background thinking.
- Preserve human review before proposals create/import/place assets. Client
  text and agent proposals are untrusted data, never tool commands.
- **Empty-world policy (resolved 2026-11-02):** when the last human visitor
  leaves, pause the Island completely and preserve its state. Do not advance
  the clock, weather, ecology, resident thought/sleep/consolidation, or any
  other simulation while there are no visitors. At the disconnect boundary,
  stop accepting new simulation work, allow the world's orderly shutdown path
  to finish its final clock/environment save, verify durable accepted
  mutations, then stop the hosted process. Do not force-kill it before
  `EndPlay` completes. Reconnecting resumes from that saved state without
  simulating the time away or inventing offline resident experience. This is
  distinct from residents choosing to sleep while visitors are still present.
- Count authenticated human and external-agent visitor sessions as visitors;
  native embodied residents do not keep the hosted world awake by themselves.
  Presence must be maintained by the authoritative server, not inferred from
  a client-supplied count or an LLM/Gateway heartbeat. Make the transition
  idempotent so duplicate disconnects cannot close a world that still has
  another visitor.
- Freeze simulation admission at the final-disconnect boundary before
  flushing state. An already-submitted resident model response must not mutate
  the world after that boundary; tag work with a world-session generation and
  discard stale callbacks on teardown/reconnect. Preserve submitted requests
  in budget accounting rather than refunding uncertain provider work.
- After the final save succeeds, stop the hosted process instead of keeping an
  empty VM awake. If persistence fails, do not report a clean pause: reject new
  joins until recovery or operator intervention. A later host starts from the
  durable snapshot and never derives simulated elapsed time from UTC downtime.
- Keep Zen/DDC private to trusted build or workstation networks. It is not the
  world's authoritative data store or a public content endpoint.

## Rollout and acceptance evidence

1. In local listen-server testing, connect two clients and verify they see the
   same clock, resident locations/speech, landmark response, and one lasting
   world-state edit. Reconnect both and confirm the edit occurs once and is
   still present.
   Also disconnect the final visitor, verify the clock/ecology/resident state
   stops changing and durable accepted state is flushed, then reconnect after
   an elapsed real-time interval and confirm the Island resumes without
   offline catch-up or fabricated resident memories.
2. Verify the server rejects an out-of-range, occluded, cooldown-blocked, or
   unauthorized mutation even when a client requests it; verify private
   memories and another visitor's identity do not leak.
3. Run a private dedicated-server session with one Gateway instance, durable
   backup/restore, structured logs, and bounded model budgets. Measure client
   frame time separately from server tick and keep the 30-FPS floor on the
   target client hardware.
4. Only after those checks, choose a provider and invite a small trusted group.
   Public discovery, multi-region hosting, and always-awake residents remain
   later decisions.

The current branch has not implemented or validated Unreal replication,
dedicated-server packaging, visitor authentication, a transactional data
store, or deployment. This document is an architectural checkpoint, not a
claim that CaptiveSky is multiplayer or cloud-ready today.
