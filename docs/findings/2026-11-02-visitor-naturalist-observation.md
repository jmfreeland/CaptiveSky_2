# Visitor naturalist observation (2026-11-02)

When a visitor presses Interact with no nearby landmark or animal, the controller
now cycles through available nearby tracks, visible vegetation, and local
weather observations. The plant note reuses the species naming shared with
resident perception, checks up to three nearby visible vegetation patches
within 650 cm, and explicitly leaves the plants growing and undisturbed. It is
descriptive only: it does not harvest, write memory or world state, or call a
model. Patch discovery happens only on interaction, not every frame.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` Development target built in the
isolated C: worktree. The focused `CaptiveSky2.Agent.NearbyVegetationPerception`
automation passed under NullRHI with Zen and Python disabled. It covers the
visitor-facing name and no-disturbance wording as well as existing resident
species perception; its 3,600-tree broadleaf fixture averaged 0.297 ms over 10
warmed NullRHI samples. Log: `Saved/Logs/Codex_VisitorVegetation_NullRHI_Final.log`.

This does not yet verify the player's on-screen caption in PIE or a rendered
view. The automation fixture also logs missing optional Fab and generated plant
assets because the worktree has no `Content/Maps/Island.umap`; these did not
prevent the focused test from passing. Next visual check: in a playable Island,
press E in a quiet patch with several nearby plant species and confirm the
caption cycles between available observations without affecting vegetation.
