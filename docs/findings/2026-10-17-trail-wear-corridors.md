# Foot-traffic wear corridors

`UIslandTrailSubsystem` already records durable wear where residents and
visitors walk. Previously every refreshed wear cell rendered as an independently
rotated round patch, so even a frequently walked route did not read as a path.

Wear patches now inspect the eight neighboring ledger cells. A small weighted
2D covariance estimates the local travel axis; anisotropic neighborhoods orient
and gently elongate the existing patch along that axis. Balanced junctions stay
round, and isolated cells retain a stable hash-derived rotation. The transform
roughly preserves patch area by widening one axis while narrowing the other.

The work remains bounded: it reuses the existing capped set of wear instances,
adds no actors/components or persistent data, and makes no model requests. Each
active patch checks at most eight neighbors when the existing wear layer is
refreshed.

## Validation

- UE 5.8.3 isolated scratch editor target with its `Source` junction pointing to
  the working tree: build succeeded and linked the updated module.
- The build also compiled `IslandTrailTests.cpp`, including coverage for a
  straight route, a diagonal route, a round cross-junction, and deterministic
  fallback orientation.
- The focused automation test was not run. Existing crash evidence in
  [`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md)
  records a repeatable null-access startup crash before project initialization,
  and the available local UE symbols/debugger are insufficient to diagnose it.
  The editor could not be brought to the foreground during this session, so no
  in-world visual check is claimed.

## Follow-up

After the UE startup crash has a symbolized diagnosis, run
`CaptiveSky2.Agent.IslandTrail` and inspect a route with intersecting resident
and visitor traffic. The current tests validate the orientation calculation,
not the final material appearance on varied terrain.
