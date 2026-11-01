# Listening Stones: residents can teach a witnessed practice (2026-11-01)

Residents who directly witnessed a quiet visit can optionally share the
`quiet-listening` practice in a nearby in-world conversation. The learner
receives one personal, persistent observation tagged
`practice:quiet-listening`, attributed to the resident who told them. The text
distinguishes something learned in conversation from something the learner
personally witnessed.

The handoff uses the existing bounded, mutually reserved conversation path. A
lesson is accepted only for the exact offered ID, only if the speaker already
has the witnessed-practice memory, and only after range, reservation, wake,
turn-limit, and queue-cap checks pass. It does not teach through player or
Discord messages, does not create shared world-state culture, and does not add
a model request. Recipients who already hold the practice are not given a
duplicate memory.

The `Agent.CraftTeaching` automation now covers the exact ID and prompt,
delivered attribution after reloading the learner's memory file, a speaker
without the witnessed tag, activation of the existing quiet-listener etiquette
choice, invented IDs, and out-of-range delivery. The UE 5.8.3 editor target
compiled successfully,
including the modified social-teaching test translation unit. The automation
was not executed: this worktree's prior `-NullRHI` commandlet attempts crashed
before automation initialization, and the open D: editor is a separate project
whose module does not contain these worktree changes. Treat runtime behavior
and model-request pacing as unverified until a safe test session can load this
build.

Next validation: load the built worktree module in a stable editor automation
session and run `CaptiveSky2.Agent.CraftTeaching`; then observe one ordinary
bounded conversation between residents in which the teacher learned the
practice from a directly witnessed visit. Confirm the recipient may choose
whether to adopt it and that no extra model request is created.
