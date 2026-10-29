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

## Saved Island trail snapshot (2026-10-29)

The project-root `WorldState/Island.trails.json` was last written at 00:58
workstation time. Its Island-day-6 ledger has 81 occupied 1.5 m cells across
four 200 m tiles: 47 have one recorded step, 34 have at least two, 20 meet the
rendered-wear threshold of four, and one has reached full wear (40 steps).
The 20 rendered-wear cells form three 8-neighbor components; the largest has
13 cells and two components have at least three. This confirms the saved
behavior has produced at least one connected corridor-shaped cluster without
adding data or actors beyond the existing bounds. It is a ledger analysis,
not proof that the cylinder marks read as a path in the live view, nor a
performance measurement. Use the largest existing cluster for the next
bounded visual review rather than adding synthetic foot traffic to the
canonical world.
