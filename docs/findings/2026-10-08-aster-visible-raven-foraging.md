# Residents notice the Raven's visible foraging clue (2026-10-08)

When the nearest unobstructed Raven is within 3.5 m and visibly carries its
fallen-twig bundle, a resident's next already-scheduled situation summary notes
the bundle without claiming to know the bird's plan. The cue is omitted at the
broader 25 m Raven-activity range, when the bundle is absent, or when solid
visibility-blocking geometry hides the bird. It is transient perception only:
no interaction, memory, world-state write, movement target, or extra model
request is added. Aster and another nearby resident may choose their own
response when their ordinary thought occurs; neither is assigned one.

UE 5.8.3 `CaptiveSky_2Editor` built successfully in the isolated
`Codex_RavenCrabAttention_20261008` scratch project. Headless
`CaptiveSky2.Agent.RavenPerch` passed under NullRHI with Python and resident
thinking disabled, zero model requests, a 60-second cap, and an isolated data
root: [`Codex_RavenVisibleForaging_retry_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenVisibleForaging_retry_20261008.log).
The updated regression verifies the cue at 3 m, its absence at 4 m while the
broader Raven activity cue remains visible (and again at 16 m), plus suppression
by solid cover. It passed in [`Codex_RavenCarryFix_Agent_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenCarryFix_Agent_20261008.log).

The first real-RHI capture showed the carried sticks floating above the Crow.
The imported rig has a head bone but no beak bone; the old local-axis offset
placed the bundle about 18 cm above the head. The presentation now maps the
raven's forward/right/down directions into head-bone space and positions three
shorter, slightly separated sticks ahead of and just below the head. The
geometry regression checks that placement, and
`CaptiveSky2.Visual.RavenWingMotion` passed in
[`Codex_RavenCarryFix_VisualFinal_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenCarryFix_VisualFinal_20261008.log).
The close 3 m capture makes the bundle legible; it becomes hard to distinguish
at 6 m and beyond, which supports keeping the narrative cue close-range rather
than describing it from a distance. The capture is a controlled blockout test,
not proof of natural perception in the finished landscape:
[`04_CarryingTwigs.png`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Viewpoints/RavenWingMotion_/20261008_211908/04_CarryingTwigs.png),
[`07_CarryingTwigs_3m.png`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Viewpoints/RavenWingMotion_/20261008_211908/07_CarryingTwigs_3m.png),
[`08_CarryingTwigs_6m.png`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Viewpoints/RavenWingMotion_/20261008_211908/08_CarryingTwigs_6m.png).
Residents still receive only a perception fact: the regression does not show
that Aster will elect to approach, speak, or assist.
