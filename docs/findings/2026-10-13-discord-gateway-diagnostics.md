# Discord gateway connection diagnosis — 2026-10-13

The gateway's readiness check passed: the Raven Discord token and configured
headless LLM key are present. A bounded live connection attempt still failed
before the Discord client reached its ready state. The first warning showed an
`HttpRequestException` wrapping a `SocketException`; after adding safe socket
classification to the adapter log, the exact result was
`HostNotFound/11001`. This points to name resolution/network availability on
the machine, not an absent configured token. No Discord message was sent. The
attempt was stopped rather than left in a reconnect loop.

`DiscordChannelAdapter` now includes the Discord log source, exception type
chain, and (for socket exceptions) symbolic/native socket error codes. It does
not log exception text or credentials. The gateway built with zero warnings
and errors to an isolated ignored output directory, and its read-only
`--ready` check passed. A normal gateway session remains unverified until DNS
can resolve Discord's gateway host.

This only concerns Raven's existing gateway route. Direct live access for
Architect still needs its separate bot credential and an explicitly
allow-listed shared guild/channel. The saved Raven conversation remains a
local snapshot, not a live Discord read.
