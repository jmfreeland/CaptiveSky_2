# Shared resident proposal quota — 2026-10-16

The Unreal `IslandAssetRequestQueue` already caps each resident at three
pending proposals and the shared inbox at 64. Discord's `AssetProposalQueue`
used the same inbox and global cap but did not enforce the per-resident limit,
so an otherwise authorized Discord correspondent could bypass the safeguard by
submitting distinct proposals faster than a human could review them.

The gateway now rejects a fourth pending proposal for the same resident with a
clear wait-for-review message. Accepted, declined, and generated records do
not consume the pending quota. The cap applies across new-object and upgrade
proposals; submissions remain review-only and do not generate, import, or
change anything in the Island. A separate resident can still submit while the
first resident is at their cap, subject to the existing shared 64-item limit.

## Verification

The gateway checks now cover the fourth-request rejection and that another
resident remains eligible. Run them with `CAPTIVESKY_GATEWAY_CHECKS_ROOT` set to
a project-local writable scratch directory when sandboxed; the test suite uses
atomic file renames, which this machine's restricted sandbox denies. The
existing checks executable passes with approved local filesystem access.

The two proposal producers still run in separate processes. Their read/check/
append sequences are not protected by a shared inter-process lock, so a truly
simultaneous Unreal and Discord submission could race the count. If concurrent
proposal arrival becomes common, introduce one cross-process lock protocol in
both producers before claiming the quota is race-proof.

The already-running gateway was left untouched because it holds its executable
open; it continues using the prior build until the next authorized restart or
logon. The new cap is compiled and checked in isolated scratch output, not yet
hot-loaded into that live process.
