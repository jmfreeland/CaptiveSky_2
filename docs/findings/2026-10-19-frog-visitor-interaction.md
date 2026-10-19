# Common frog visitor interaction

## Change

The common frog now resolves through the ordinary visitor interaction target as
`TideglassFrog`. Visitors may quietly observe the animal; if it is settled and
the observer is within its existing startle envelope, the frog makes its
existing short hop toward cover. A hop already in progress or its existing
startle cooldown produces a quiet, no-change response instead. The frog remains
wild and cannot be selected as a movement target.

The interaction uses no LLM call, collection/feeding behavior, persistent
memory, or world-state write. It adds no new polling/tick work; it routes through
the existing target discovery and interaction path.

## Verification

`CaptiveSky2.Agent.CommonFrogVisitorInteraction` creates an isolated synthetic
world with collision ground and a visitor. It verifies target discovery,
non-movement status, a real transient hop through the shared `Perform` path,
and idempotent repeated observation. The UE 5.8.3 editor target built in an
isolated no-content scratch project, and the focused automation test passed
under NullRHI. The test also checks that the player proximity hint says
"quietly watch a wild common frog," and that fox, stag, periwinkle, and
dragonfly targets receive habitat-appropriate observation hints rather than
internal-tag fallbacks. The fixture checks visibility before adding its
synthetic hop floor, so the floor cannot mask the target line-of-sight
assertion. The scratch log contains expected missing-Content asset warnings
and does not prove a natural Island encounter; a real-play camera review
remains outstanding.

The 2026-10-19 wildlife-hint extension builds successfully in the same scratch
target, and `CaptiveSky2.Agent.CommonFrogVisitorInteraction` passes under
NullRHI. The fixture now registers a scene root on its generic visitor actor
and keeps its actors away from subsystem-created origin actors, so the
distance and nearest-target checks exercise the intended frog. The scratch log
still contains expected missing-Content asset warnings and is not a visual
Island encounter review. No change was made to the live editor or Island map.
