# Separate authenticated visitor contributions (2026-11-02)

The visitor guest book and stone-arrangement ledger already persisted author
IDs and enforced one contribution per Island day, but the player controller
hard-coded every human as `Visitor`. In a shared server, that made people share
one quota and made their persistent work indistinguishable. Replacing this
with a client name or a per-connection ID would be misleading: names are
untrusted, and reconnecting would reset the latter.

Online play now derives a deterministic, Island-scoped pseudonym from the
server-provided `PlayerState` unique ID. It stores only that pseudonym in the
guest book and arrangement records; the raw account ID is not written there.
This pseudonym is a public per-Island label, not anonymization or an identity
proof; security still depends on the configured online provider supplying a
verified `PlayerState` unique ID.
Local/offline sessions keep the existing `Visitor` author. Guest-book UI names
the signer, and its write path now accepts the visitor identity used by the
daily-limit check. Regression assertions cover stable same-Island identity,
separation between accounts and Islands, local fallback, and non-disclosure of
the raw account string.

This does not add or configure an authentication provider. Until a hosted
deployment supplies verified online IDs, the existing design requirement
stands: do not claim per-person hosted contribution limits, and do not enable
public contributions without identity validation. The UE 5.8.3 Editor target
compiled and linked successfully (192 actions). Automation and runtime
multiplayer tests remain to be run; next test two authenticated clients
creating independent guest-book and arrangement contributions across
reconnect.
