# Live editor video-memory pressure (2026-10-08)

The current Island editor viewport displayed Unreal's red warning:
"Video memory has been exhausted (203.125 MB over budget)." At the same time,
`nvidia-smi` reported 12,282 MiB total, 11,341 MiB in use, and 657 MiB free on
the RTX 4080 Laptop GPU. Windows GPU-process counters showed two
`UnrealEditor.exe` processes, PIDs 828 and 24160, using 8,592.6 MiB and
2,555.8 MiB of dedicated GPU memory respectively. Their dedicated-memory
figures total 11,148.4 MiB, close to the system-wide used-memory figure.

This makes concurrent editor processes a strong explanation for the visible
video-memory over-budget warning. It does not establish that either editor
caused the separate `dotnet.exe` CLR exception (`0xe0434352`): the dialog is
generic, and no matching Windows Application event was available in the
read-only query. The Unreal MCP reported PIE was not running. Neither editor
was closed because their unsaved state and project command lines could not be
verified.

## Safe next check

After confirming which editor window is safe to close, keep one editor open and
recheck `nvidia-smi` plus the viewport warning. Then compare the warning with
the Windows Application log's matching `.NET Runtime` / `Application Error`
event, if present. Treat these as separate symptoms unless a crash record or
reproduction ties them together. Avoid another heavy PIE run while the GPU is
this close to its memory budget.
