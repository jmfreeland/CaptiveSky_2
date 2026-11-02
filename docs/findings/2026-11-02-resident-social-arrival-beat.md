# Give resident approaches a timely social follow-up (2026-11-02)

In the bounded 15-minute scratch run, Aster chose to approach Raven at
03:49:47, then chose another Raven approach about four minutes later with the
thought that a quiet greeting could begin a conversation. The controller
reported that Aster was already at a reachable ground position near Raven.
The log has no automatic-conversation delivery for that encounter. Aster's
next background decision was scheduled on the normal four-minute interval, so
the social moment was not reconsidered promptly. This is a single NullRHI run,
not a rendered or general-frequency claim.

After a successful approach to another resident—or a movement request that
finds the resident already at speaking distance—the controller now advances
the next already-scheduled autonomous thought to a 45-second quiet beat. It
does not create an extra LLM request, override a closer existing think time,
or automatically make anyone speak. The normal situation, per-resident
request spacing, shared real-time/request caps, conversation reservation, and
recipient choice remain authoritative. Landmark movement, failed approaches,
and unrelated movement tags do not trigger the social timing adjustment.

The `CaptiveSky2.Agent.ResidentApproach` regression covers resident-tag
filtering and deadline behavior, including retaining an earlier scheduled
thought and avoiding an immediate request from the movement callback. The UE
5.8.3 isolated Editor target compiled and linked successfully (37 incremental
actions). Automation remains unrun because the known early Unreal startup
crash is unresolved; the next useful evidence is a bounded resident session
that checks whether an approached pair chooses to speak without exceeding the
existing request cap.
