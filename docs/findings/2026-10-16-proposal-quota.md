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

The quota-only change initially left the two producers' read/check/append
sequences open to a cross-process race. The follow-up below adds a shared lock;
runtime interoperability still needs an editor automation run before calling
the entire Unreal/Discord path behavior-verified.

The already-running gateway was left untouched because it holds its executable
open; it continues using the prior build until the next authorized restart or
logon. The new cap is compiled and checked in isolated scratch output, not yet
hot-loaded into that live process.

## Follow-up: cross-process serialization — 2026-10-16

Both producers and the gateway's proposal readers now take an exclusive handle
on the same sidecar, `inbox.jsonl.lock`, before reading or changing the JSONL
inbox. The Discord gateway uses .NET `FileShare.None` and waits asynchronously
up to five seconds. Unreal uses a Win32 handle opened with share mode zero, waits
up to 50 ms, then fails closed with a retry message to avoid a long game-thread
stall. The lock file may remain on disk; ownership is the open handle, so a
process exit releases it without stale-lock cleanup.

The full gateway checks pass, including a separate-process contention test that
holds the exclusive handle while Discord waits, then confirms append succeeds
after release. The isolated UE 5.8.3 `Claude_Props` Editor target compiled and linked
the C++ queue and its new lock-contention automation test (13 actions); the
main project's loaded DLL was not modified. The automation itself was not run:
the editor remained unavailable, and earlier unchanged commandlet retries had
exited without a log. Thus C# contention behavior is executed evidence, while
Unreal runtime behavior and C#/Win32 interoperability remain to be exercised
when the editor safely recovers. The live gateway process was not stopped and
continues on its previously loaded build until an authorized restart.
