# Resident request playtest (2026-10-16)

The isolated 10-minute spectator run did not reach the Island. The freshly built
scratch project launched the game process, which exited during engine startup
before the script's world-ready marker. Unreal wrote a crash report at
`Saved/CompileScratch/Codex_ResidentRequestQueue_20261016/Saved/Crashes/_0000`:

- `EXCEPTION_ACCESS_VIOLATION` reading null (`0x0`)
- `SecondsSinceStart=0`
- crash stack hash: `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`

No resident model requests were made and no test inbox was created. No world
changes were applied. The main editor was left untouched; the scratch launch
crash is consistent with the project's unresolved zero-second startup crash,
but has a different stack hash from the earlier recorded failure and needs a
separate symbolized diagnosis before assuming they share a cause.
