# Validate the embodiment handoff lease (2026-11-02)

The Gateway now accepts `.gateway/embodiment.json` only when its schema is
version 1, its `agent_id` matches the configured resident, its `session_id` is
non-empty, and its lease has not expired. An unknown schema or missing session
identity fails closed, so malformed or future-format presence cannot cause
Discord turns to be routed to an embodiment the Gateway cannot reliably
identify.

The Gateway checks cover a valid lease, an unsupported schema, a blank session
ID, normal embodied turn handoff, and fallback when a lease expires. The full
Gateway check executable passed. Restore emitted NU1900 warnings because the
environment could not reach NuGet's vulnerability-data service; package
restore/build and the checks themselves completed successfully.

This file is an ownership lease for one Unreal body, not proof that an external
agent is visiting the Island. Discord presence and messages must not count as
visitors for hosted empty-world policy. A future agent visitor needs a separate
authenticated session issued and renewed by the authoritative host.
