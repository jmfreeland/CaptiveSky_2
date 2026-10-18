# Resident proposal target validation — 2026-10-18

The Unreal request queue previously rejected CR/LF in proposal targets but
allowed other control characters. The reader already fails closed on every
control character, so a tab or a non-whitespace control byte accepted by the
writer could make the shared `inbox.jsonl` unreadable to later reviewers and
requests.

`AppendRequest` now rejects every `FChar::IsControl` target character before
acquiring or writing the queue record. Automation coverage checks both a tab
and a non-whitespace control character, verifies the user-facing single-line
error, and confirms that rejected proposals leave the inbox byte-for-byte
unchanged. Normal descriptive object, landmark, and world-level targets remain
supported.

This is a source-level safeguard for the human-reviewed resident request flow;
it does not execute generators, import assets, or change the world. The UE 5.8.3
scratch target build succeeded (5 actions); both the implementation and focused
automation test compiled and linked. Runtime automation was not run: earlier
full-content commandlet attempts crash during Core startup before reaching tests,
so there is no runtime pass claimed for this change.
