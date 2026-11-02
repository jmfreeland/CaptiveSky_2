# Captive Sky — Weekly Worldbuilding Ideas

## Hosting rule — pause the Island when empty

When the last visitor leaves, the hosted Island pauses and preserves its state.
Its clock and ecology do not advance offline; conscious residents enter their
sleep state without experiencing elapsed offscreen world time. On return, the
Island resumes from the preserved state. This keeps offline hosting from
silently aging the world or creating events while nobody is present.

## 2026-11-02 — Groundcover still needs stronger visitor-facing composition

The current live editor viewport shows scattered groundcover tufts over broad
exposed brown soil, with a large foreground rock dominating the frame and no
canopy visible in that camera slice. This is only a local editor observation,
not an Island-wide or runtime verdict. Next validate a deliberate Tideglass-to-
woodland route at player height before changing global density; pair the visual
review with frame-time and whole-device memory measurements. See [live
groundcover observation](findings/2026-11-02-live-editor-groundcover-observation.md).

## 2026-11-02 — Soften bare soil without increasing meadow budgets

The broad-meadow field's open-glade acceptance floor rose from 8% to 14%,
while its nested-patch strength was recalibrated to keep mean acceptance near
46%. This should soften the largest bare patches without adding a plant layer
or increasing the candidate ceiling; lush masses and visible glades remain.
The UE 5.8.3 Editor target compiled, including the focused mean/contrast
regression, but the test was not run because of the known zero-second startup
crash. A matched, player-height visual and performance check is still needed.
Details: [open-glade cover](findings/2026-11-02-groundcover-open-glade-floor.md).

## 2026-11-02 — Keep dragonfly flower visits light at Island scale

The nearby blooming-flower query now uses the 15 m flower-cell index already
built for local foliage sway instead of checking every flower instance across
all 11 species on each pollinator check. It preserves the existing bloom,
visibility, and exact world-distance rules, with a full-scan fallback for
uncached previews or unusually wide queries. The focused seasonal-flower
fixture covers indexed lookup across a cell edge, translated components,
negative world coordinates, and the fallback. UE 5.8.3 compiled and linked the
Editor target; automation remains unrun because of the known early Unreal
startup crash. This is a source-level workload reduction, not a measured FPS
claim. Details: [pollinator flower spatial query](findings/2026-11-02-pollinator-flower-spatial-query.md).

## 2026-11-02 — Let visitors find the minnow's recent ripple

The visitor's existing on-demand `E` nature cycle now includes the same
time-limited minnow surface-break note residents receive, including relative
direction and distance. It is only offered within the existing habitat range;
the ripple itself remains brief, and the cue adds no recurring scan, target,
interaction, or persistent change. The UE 5.8.3 Game target compiled with a
focused visitor-caption regression, but automation remains unrun until the
known startup crash is diagnosed. Details: [visitor minnow event observation](findings/2026-11-02-visitor-minnow-event-observation.md).

## 2026-11-02 — Let residents locate the minnow's brief ripple

The next already-scheduled resident action context now carries the actual
position of a recent minnow surface break and describes it by approximate
distance and relative direction. The splash and its short-lived nature remain
unchanged; this adds no tick, poll, model request, lasting memory, or movement
target. The isolated UE 5.8.3 Game target compiled with a focused context
regression, but Unreal startup remains unstable, so runtime automation and
behavior are not yet verified. Details: [minnow event wayfinding](findings/2026-11-02-minnow-event-wayfinding.md).

## 2026-11-02 — Help visitors find the plant they notice

On-demand plant observations now identify the nearest visible patch by
approximate distance and relative direction, so a visitor can look toward the
named species instead of receiving only a vague “nearby.” The observation uses
the already-selected visible instance and adds no scan or rendering work. The
isolated UE 5.8.3 Game target compiled successfully, including the focused
regression source; runtime caption verification remains blocked by the known
zero-second Unreal startup crash. Details: [visitor plant wayfinding](findings/2026-11-02-visitor-plant-wayfinding.md).

## 2026-11-02 — Give Black Alder a damp woodland edge

The five-species broadleaf field now uses the existing Tideglass wetness
gradient to form coherent Black Alder patches near the pool, leaving the
existing species and instance budgets untouched. The isolated UE 5.8.3 Game
target compiled successfully, including the new focused regression; its Editor
automation and rendered appearance remain unverified. Next compare the same
Tideglass-to-woodland view when a stable editor test run and the GPU reserve are
available. Details: [wet-edge broadleaf affinity](findings/2026-11-02-wet-edge-broadleaf-affinity.md).

## 2026-11-02 — Keep RatHollow placement tied to a fresh Island audit

The active D: Island state currently has no `wildlife_homes` record, and the
newest named RatHollow site-review audit I found is from October 16. It is too
old to satisfy the V9 review workflow's 24-hour gate, so no candidate location
is approved and the live map remains unchanged. The full 64-test
`Scripts/tests` suite passes under Blender 5.2.2's bundled Python, including
the review script's stale/mismatched-site refusals and source-map protection.
Next, obtain a fresh qualified `Island.RatHollowSiteAudit` result in the
correct project, then inspect only a copied-map placement for Rat-scale access,
concealment, ground contact, collision, and navigation before creating any
canonical home record. See the [V9 review workflow](findings/2026-11-01-rathollow-hidden-v9.md).

## 2026-11-02 — Let visitors feel the lee of the Island's landmarks

The on-demand `E: notice your surroundings` cycle now includes the same
conservative six-metre upwind obstruction check used in resident nest-site
scouting. It can tell a visitor that the present horizontal wind is blocked or
exposed, and explicitly says this does not establish roof cover or a safe
perch. Calm conditions suppress the note. The UE 5.8.3 Game target built and
compiled the focused regression source; runtime automation and a rendered
caption check remain pending. Details: [visitor wind-shelter observation
](findings/2026-11-02-visitor-wind-shelter-observation.md).

## 2026-11-02 — Let ground-cover communities follow less regular edges

The broad seeded ground-cover species field now bends through two smooth,
independent 24 m noise fields, displacing the sample by up to 9 m per horizontal
axis. This keeps the existing species and instance budgets unchanged while
giving community boundaries room to meander. The UE 5.8.3 Game target built
successfully. A fixed-seed regression now covers boundary variation and local
species coherence, but UBT would not compile the Editor target while Live
Coding is active. A matched visual check is still needed when GPU headroom
permits. Details: [ground-cover community warp](findings/2026-11-02-groundcover-community-warp.md).

## 2026-11-02 — Let the meadow change with the seasons

The eight generic Fab flower forms now open and fade in overlapping windows
across spring, summer, and autumn, with three-day transitions and a subtle
stable spatial stagger. This keeps spring, midsummer, and late-autumn meadows
from looking identical without increasing the foliage count or introducing
per-frame updates. The forms are not botanically identified, so these are
visual habitat rhythms rather than asserted species calendars. The on-demand
visitor nature note can distinguish flowers beginning to bloom from open
flowers. UE 5.8.3 built successfully; focused NullRHI seasonal and nearby
vegetation-perception automations passed. Rendered appearance remains pending
the 4 GiB GPU reserve. Details: [seasonal meadow phenology](findings/2026-11-02-seasonal-meadow-phenology.md).

## 2026-11-02 — Let visitors quietly notice nearby wildlife

When a visitor uses the on-demand outdoor observation cycle without a close
interaction target, it can now name the nearest visible IslandLife species
within 18 m: stag, fox, firefly, shore crab, periwinkle, dragonfly, frog, or
minnows. The note explicitly leaves the animal undisturbed; it neither moves
the wildlife nor changes memory/world state or calls a model. Discovery is
interaction-triggered, not a background scan. The focused UE 5.8.3
`CaptiveSky2.Agent.VisitorWildlifeObservation` NullRHI automation passed for
visible, hidden, and distant cases. This is behavior evidence only, not proof
of species readability in the rendered Island. Details: [visitor wildlife
observation](findings/2026-11-02-visitor-wildlife-observation.md).

## 2026-11-02 — Confirm the five-species woodland scatter in a live scratch run

The guarded 15-minute scratch Game run actually materialized the worktree's
five broadleaf canopy species at 720 each, across 372/500 groves, while
holding the 3,600-tree budget fixed. The separate on-disk D: checkout still
declares four broadleaf species and lacks Common Hazel; its broadleaf cull
ranges also end well before spruce's 550 m horizon. That can make a broad
view read as spruce-only, but remains a distance-based inference until a
rendered view is inspected. Keep the count/cull settings unchanged until
headroom permits a matched visual/performance check. Details and the exact
runtime census are in the [15-minute resident finding](findings/2026-11-02-resident-feedback-15m.md#runtime-vegetation-census).

## 2026-11-02 — Make generic object-upgrade proposals easier to target

Visible ordinary static-mesh props now include resident-relative direction as
well as distance in the existing optional `request_upgrade` context. This
helps distinguish two similar props with the same mesh-derived label; the
scan remains local, line-of-sight checked, and capped at the three nearest
objects. Upgrade requests can still name stable IDs, descriptive locations,
or a genuinely world-level target, but are only recorded for human review.
The request-queue automation confirms all three upgrade kinds, target forms,
duplicate rejection, and the per-resident pending limit. The worktree editor
target built, and `CaptiveSky2.Agent.NearbyVegetationPerception` passed in the
isolated `Saved/CS58` project under NullRHI with thinking disabled and zero
model requests. This validates text targeting, not rendered recognition of
individual props. Two subsequent isolated sessions exposed the new relative
labels in residents' contexts. The first ended early under the RAM safety
guard; the second completed 15 minutes, but residents chose conversation, rain
observation, and stone arranging rather than an upgrade proposal. This is
useful agency evidence, not a failed upgrade-feature test. Details:
[object-target feedback follow-up](findings/2026-11-02-resident-feedback-object-targets.md).
Log: `Saved/CS58/Saved/Logs/Codex_UpgradeTargetPerception_Final_20261102.log`.

## 2026-11-02 — Let visitors notice the changing sky

The on-demand `E: notice your surroundings` cycle now includes the Island's
existing dawn/dusk horizon description or the current night-sky description
when those conditions apply. The note reads the live clock and cloud cover,
does no per-frame scan or persistent write, and is suppressed under the inn
roof. A focused controller automation covers clear night, seasonal sunrise,
indoor suppression, and midday without claiming visible stars. The UE 5.8.3
editor target compiles and the focused NullRHI automation passed. Details are
in the [sky-observation finding](findings/2026-11-02-visitor-sky-observation.md).

## 2026-11-02 — Ask residents what felt alive after a short Island visit

A guarded 15-minute NullRHI session reached world-ready and ended normally
with 16 of 32 allowed model requests. Rat, Raven, and Aster each chose to
share a brief reflection about the wet Tideglass edge—the minnow's surface
ripple, rain rings, and a circling dragonfly. The Innkeeper chose not to
speak. No object or upgrade requests were made, and all playtest state remains
in a fresh isolated data root. Their repeated attention to small water
movement is a useful art-direction signal, not rendered-visibility proof.
In a second guarded 15-minute run, all four residents answered the check-in;
the Innkeeper floated a leaf in the basin and Rat hoped to see Tideglass under
moonlight. The same run recorded a stable ~5.4 GiB Game private-memory
footprint, and all data stayed isolated. Details:
[15-minute resident feedback](findings/2026-11-02-resident-feedback-15m.md) and
[object-target feedback follow-up](findings/2026-11-02-resident-feedback-object-targets.md).

## 2026-11-02 — Review Tideglass under moonlight

Rat's request to see Tideglass by moonlight is a concrete art-direction cue.
The source already provides a lunar-phase-, night-, and cloud-responsive
directional moon, while the transient pool material has specular response and
a restrained edge-reflection tint. That makes moonlight response plausible,
but is not visual proof. Current free GPU memory is about 1,981 MiB of 12,282,
below the project's 4 GiB rendered-run reserve, so no renderer was started.
Next: when the rendered-memory preflight passes, capture a matched moonlit
Tideglass view and judge whether the water, shore, and surrounding species
actually read as a place worth visiting after dark. Keep the 30-FPS floor and
do not add another shader effect until the image shows what is missing.

## 2026-11-02 — Let visitors read Tideglass's changing waterline

When a visitor uses Interact without a nearby target, the environmental notes
now include the Island clock's current lunar tide when Tideglass is within
12 m. The note is on-demand, read-only, and uses the same tide model as the
pool; it makes no model call or persistent change. The no-target action also
has a generic outdoor hint, so it remains discoverable on clear, dry ground
without scanning the world on the UI refresh timer. UE 5.8.3 compiled and
linked the controller and focused near/far test. The headless run hit the
machine's known zero-second Unreal startup access violation before test
discovery, so runtime behavior remains unverified. Retry only after that
bootstrap issue is diagnosed or the startup context materially changes. Details:
[visitor naturalist observation](findings/2026-11-02-visitor-naturalist-observation.md).

## 2026-11-01 — Help visitors discover resident conversation

The local interaction hint already described nearby objects, wildlife, trails,
and weather, but the live `Enter` shortcut for talking to Aster, Raven, or the
Innkeeper was only explained in the README. Desktop visitors now also see a
quiet `Enter: talk with <name>` hint when a resident is within conversation
range. It does not move or prompt the resident, begin a model call, or force a
conversation; touch-control sessions do not advertise a keyboard-only key.
The UE 5.8.3 compile-only build succeeded for the controller and focused hint
test code. Runtime automation and an in-world UI check remain pending while
the separate D: editor is open. Details: [visitor conversation hint](../Source/CaptiveSky_2/CaptiveSky_2PlayerController.cpp).

## 2026-11-01 — Keep native ocean surfaces from acting like solid ground

The active D: project had the legacy hidden `OceanPlane` collision disabled,
but did not yet change Pawn blocking on generated WaterBodyOcean hulls or the
WaterZone mesh. A narrow merge now updates those responses to overlap while
preserving the water query behavior, and extends the existing native-ocean
fixture with a regression assertion. UE 5.8.3 compiled the subsystem and the
updated test; final linking was blocked by the editor holding its DLL, so the
automation and actual traversal remain unverified. Do not terminate the editor
or start another renderer to force this check. Next: after a safe editor reload,
run `CaptiveSky2.Agent.IslandOcean` and test an actual player entry into the
water. Details: [active-project water collision merge](findings/2026-11-01-live-checkout-parity-and-zen-startup.md#active-project-water-collision-merge-2026-11-01).

**Worktree validation (2026-11-02):** in the isolated `Saved/CS58` project,
`CaptiveSky2.Agent.IslandOcean.RuntimeSwim` and
`CaptiveSky2.Agent.IslandOcean.SavedMapCollision` both passed under `-NullRHI`.
The fixture verifies that startup removes Pawn blocking from generated ocean
collision and that a player-sized capsule can cross open water and return to a
shore floor. This validates the worktree's runtime behavior, not the separate
D: editor's loaded module or rendered player traversal; keep that final check
pending a safe editor reload and sufficient headroom.

## 2026-11-01 — Give headless correspondents the same bounded reply horizon

The Unreal play-session path already stops after 30 real minutes or 64 shared
embodied model requests, but the Discord headless responder could previously
answer indefinitely. Headless turns now reserve from a persisted per-agent
rolling allowance of 64 replies per 30 minutes. The reservation is written
before a model call and survives gateway restarts; direct review/status
commands do not use LLM turns. At the limit, the correspondent gets an
approximate retry time, and the declined message is neither read into memory
nor remembered as experience. Corrupt or unavailable quota state fails closed.
Gateway checks pass for capacity, reset timing, restart persistence, denied-turn
behavior, deduplication, and no extra model call. Details: [Gateway README](../Gateway/README.md).

## 2026-11-01 — Bound Discord turn backlogs for multi-user hosting

Discord events previously spawned one task per inbound message, even though
the per-agent runtime lease serializes the actual turn. A burst could therefore
leave an unbounded set of waiting tasks behind the lease. Each route now feeds
a FIFO queue capped at 16 messages and processes accepted messages in arrival
order. When full, a new message is explicitly not queued and an allow-listed,
addressed sender receives a resend notice at most once per channel every 30
seconds. Unaddressed guild chatter is filtered before queue admission, so it
cannot occupy the agent's turn slots. The gateway checks cover ordering, capacity, overflow, drain, and
invalid capacity; all checks passed. The cap is per agent route and does not
change the embodied play/request limits. If more channel adapters are added,
keep the same per-agent bounded-routing boundary rather than giving each
adapter an independent unlimited worker pool. Details: [Gateway README](../Gateway/README.md).

## 2026-11-01 — Refine the spring woodland ramsons candidate

The first plain-material preview showed separate low leaf fans and small,
hard-to-read flower heads. The seven rosettes are now tighter, leaf blades
narrower, and the three leafless scapes 38–43 cm tall, in line with the
up-to-45 cm range in [Plantlife's woodland ID guide](https://www.plantlife.org.uk/wp-content/uploads/2024/06/WEB-Plantlife-ID-Guide.pdf).
The refreshed 98 × 101 × 49 cm FBX remains at 4,222 triangles and exports
collisionless with five material slots. The source contract passes in Blender
5.2.2, and the FBX re-import verifier reports one UV layer and `PROBLEMS []`.
The preview is improved but leaves still look somewhat planar; keep it
unplaced until a disposable Unreal import confirms ground contact, material
response, and visitor-scale readability. Details:
[ramsons candidate](findings/2026-11-01-woodland-ramsons-candidate.md).

## 2026-11-01 — Add a RAM guard to hidden resident-feedback runs

The 15-minute NullRHI character-feedback path now requires the existing 4 GiB
system-memory preflight and two-consecutive-sample process monitor. This lets
future resident feedback runs remain independent of rendered-GPU headroom
without leaving their RAM use unguarded. The 900-second duration cap, 64-request
hard ceiling, isolated data root, and voluntary character responses remain
unchanged; the documented invocation uses 32 requests. All 58 source tests and
the live Windows `GlobalMemoryStatusEx` boundary/monitor tests pass. No Unreal
session or resident prompt was run because the existing engine pre-init crash
has not been resolved. Details: [NullRHI feedback RAM guard](findings/2026-11-01-nullrhi-feedback-ram-guard.md).

## 2026-11-01 — Keep legacy PlantFactory out of the active project

A fresh descriptor check confirms `ProceduralVegetationEditor` remains enabled
for UE 5.8 authoring, while `PlantFactoryPlugin` is absent from
`CaptiveSky_2.uproject` and its local plugin folder is absent. The remaining
PlantFactory mentions are migration history and guarded restoration/audit
scripts, not an active runtime dependency; keep that provenance instead of
mistaking it for a plugin still in use. A regression test now protects this
boundary so PVE stays available without reintroducing PlantFactory. The new
test and all 59 `Scripts/tests` cases pass. No Unreal process, asset, or map was
changed. Details: [PlantFactory migration](findings/2026-10-12-plantfactory-migration.md).

## 2026-11-01 — Confine the Megaplant runtime-material proof project

The prepared 24-package PVE material-closure copy/rewire scripts now refuse to
run unless the active project is the dedicated
`MegaplantRuntimeMaterialSandbox`, and store/read their manifest under that
project's own `Saved/` directory. This closes a real safety gap in the harness:
the previous scripts relied on operators to choose the correct project and on
the process working directory to resolve the manifest. Three focused boundary
tests and all 62 `Scripts/tests` cases pass. The UE sandbox still has not
materialized any assets; the existing NullRHI commandlet bootstrap failure is
unchanged. Details: [Megaplant runtime material contract](findings/2026-11-01-megaplant-runtime-material-contract.md).

## 2026-11-01 — Keep Megaplant materials self-contained at runtime

The expanded UE Editor audit now covers Common Hazel as well as the original
installed tree variants. It confirms that all nine meshes still reach the
ProceduralVegetationEditor sample material closure (24 packages); the
project-owned bark parent is not yet a runtime fix because it shares that
closure. Species textures are already project-owned, while each material
instance also carries seasonal/health/translucency overrides. Preserve that
art direction when copying the full dependency graph into project content; do
not flatten it to a two-texture substitute. A read-only nested graph walk found
62 resolvable function calls and the four unique direct PVE expression assets:
three virtual textures and `MPC_GlobalFoliageActor`. The collision-checked
mapping now covers all 24 dependencies (3 textures, 1 master material, 19
functions, 1 parameter collection); every proposed destination under
`/Game/Megaplant_Library/RuntimeMaterials/PVE/` is empty and name-collision
free. All 24 source `.uasset` files exist in the installed UE plugin (2.41 MiB
total, with no `.uexp` or `.ubulk` sidecars). The plugin also has a distinct
Runtime module, so the next check must prove the copied graph works without its
Editor module and establish whether the Runtime module can also be omitted.
An isolated project and copy/remap/verify scripts are prepared, but the first
UE 5.8.3 NullRHI commandlet crashed before script execution; no sandbox assets
were created. Its crash report records a zero-second null read in the
UnrealEditor-Core/Cmd bootstrap (no installed symbols), but the current
isolated UE 5.8.3 Editor target compiles and links in 44 actions. The harness
also passes offline Python syntax checks. The build does not execute Unreal
automation, and the startup failure remains unexplained; do not retry that
commandlet recipe unchanged. No assets were changed. Next, find a materially
different safe validation route, then copy/remap only in the disposable
project and compile/preview before touching live tree parents. Details:
[runtime material contract](findings/2026-11-01-megaplant-runtime-material-contract.md).

## 2026-11-01 — Give the rain basin breathing room

Resident feedback found Rat's direct approach to the basin blocked by
StarterContent rocks. Placement now screens for at least two grounded,
capsule-clear standing spots around each candidate, reducing the chance that a
good-looking basin site is awkward to use. The focused UE automation was added
and compiled, but not run because the active editor is the separate live D:
checkout and headless NullRHI startup is currently unreliable. This does not
guarantee a clear path through the surrounding woodland; verify the detour and
interaction in rendered play when the GPU/editor gate permits. Details:
[approach-clearance finding](findings/2026-11-01-rain-basin-approach-clearance.md).

## 2026-11-01 — Add Common Hazel to the mature woodland canopy

The installed Common Hazel A mesh has a distinctive multi-stem form and a
roughly five-metre silhouette that suits the Island's temperate understory.
The worktree now treats it as a fifth spatially clustered broadleaf species,
with its own non-colliding, non-navigable instanced-skinned component and a
450–700 cm height band. The mature-tree total remains capped at 3,600; when
all five assets resolve, the per-species cap is 720. Existing species keep
their A/B variation, while Hazel uses the inspected A mesh only. Resident
observations can name Common Hazel. Next: run the focused broadleaf/Nanite
automation in the isolated project whose Content junction resolves the
installed Fab assets, then compare a rendered Island view and memory/frame-time
telemetry before syncing code to the live D: checkout. The
UE 5.8.3 editor target has now compiled and linked successfully (41 actions),
including the new component, scatter path, resident description, and tests.
The focused automation is not yet run: the active editor is the separate live
D: checkout, and the previous headless commandlet attempt failed before engine
startup. A read-only look at the live editor's actual
`Tree_Common_Hazel_01_A` Skeletal Mesh preview confirms the compact, leafy
multi-stem silhouette (3,714 LOD0 triangles; approximately 5.32 m bounds); it
is an asset sanity check, not an Island placement review. The workstation's
GPU snapshot during this review reported 10,752 MiB used, only 1,246 MiB free,
76 °C, and 46% utilization. This is below the existing 4 GiB rendered-session
gate, so no second editor or Game renderer was started. No live map or asset
was changed.

## 2026-11-01 — Prepare a no-default-site review for Rat's fallen-wood hollow

The offline-preferred V9 hollow now has a staged Unreal review workflow that
imports it to a dedicated asset/library destination, copies the Island, and
places it only in an already-open review copy. Placement requires a qualified
`Island.RatHollowSiteAudit` candidate from the prior 24 hours and an exact
coordinate match; prior Site A/B suggestions remain unapproved. Eight focused
script checks and all
58 `Scripts/tests` cases pass. The workflow has not been run in Unreal, so this
is preparation rather than a home-placement approval. Next: confirm the active
editor checkout, obtain a current qualified site audit, then review ground
contact, Rat-width access, concealment, and visitor-scale appearance before
writing any canonical home state. Details: [RatHollow V9 review](findings/2026-11-01-rathollow-hidden-v9.md).

## 2026-11-01 — Keep investigating recurring Zen cold starts without risking the cache

The user reports a slow-Zen warning on every editor launch. Archived UE 5.8.3
logs show both short and long starts (3.138–52.602 seconds), including waits
for another launch's recovery and clean cold starts. The current desktop later
showed the Island loaded with no warning visible, but this did not prove the
live project's startup configuration or explain each delay. The worktree
already has a project-scoped Zen keep-alive setting; the actual D: project
needs a deliberate, preservation-first config sync before that mitigation can
be assessed there. Do not delete shared Zen data or launch a second editor to
reproduce the warning. Details: [Zen cold-start finding](findings/2026-11-01-zen-cold-start.md).

## 2026-11-01 — Hold the landscape-material comparison until the revision arrives

A read-only check of the open D: project's `Content` tree found no landscape,
ground, soil, terrain, or material-named Unreal asset newer than the existing
October 12 Fab download; the Island landscape material assets themselves were
last saved October 10. This does not rule out a source staged outside
`Content`, but the proposed Claude landscape-material revision is not present
in the project's asset tree yet. Do not change the Island assignment or save
the map to guess at it. When the handoff path is known, preview that exact
revision in an isolated copy and compare it against the current material,
including the previously logged invalid texture-index warning (`21 / 20`),
before proposing a map change.

## 2026-11-01 — Make rendered-play retries wait for symbolized startup-crash evidence

A guarded 15-minute Game feedback launch passed its fresh 4 GiB GPU and RAM
preflights, then exited before world-ready with the same zero-second
null-address `UnrealEditor-Core` fingerprint as the earlier NullRHI startup
crash. A short, zero-request diagnostic with `-DDC-ForceMemoryCache` and
`-ddc=NoZenLocalFallback` reproduced the identical failure, so Zen/DDC is
unlikely to explain this crash (separate from Zen's recoverable slow cold
start). Both attempts stayed under the launcher supervisor; neither produced
world state or model requests. The 15-minute NullRHI feedback pass remains the
latest successful character session. Do not repeat either Game launch recipe
unchanged; symbolize the dump or validate a materially different startup path
before resuming rendered play. Details: [guarded rendered-play startup](findings/2026-11-01-rendered-playtest-startup.md).

## 2026-11-01 — Keep rendered-play safety tests aligned with the launcher

Two source-only regression assertions had drifted from the safeguards they
protect: the launcher now polls at the shorter configured GPU/RAM interval,
and its system-memory check uses `GlobalMemoryStatusEx` behind
`Get-AvailableSystemMemoryMiB` rather than querying WMI directly. The tests now
assert those actual boundaries. All 39 `Scripts/tests` cases pass, including
the GPU/RAM preflight, monitoring, process-scope, and bounded-feedback checks;
all 15 resident asset-request workflow tests also pass. The wider asset-pipeline
suite had 112 passing tests, with its separate bread-texture test unable to
import `numpy` in the available Python. No dependency was installed. This is
test-harness validation only: no Unreal build or rendered play was run, so the
water-crossing and GPU-gated visual checks remain open.

## 2026-11-01 — Let residents read the changing tide, and remember only what happened

Rat and the Innkeeper both wondered how the shore changes as the tide falls.
Tideglass and the WaterBodyOcean now follow the Island's same semidiurnal
spring/neap curve, with a subtle height offset on the actual ocean and a
rising/falling/turning cue in nearby resident perception. The time look-ahead
now crosses midnight continuously. Review of the bounded Raven session also
found a false reflection: the model remembered setting a leaf afloat in the
same response that requested the interaction, although the controller
rejected it and the saved basin had no leaves. Model memories are now skipped
for an interaction response; the controller's verified outcome can be
remembered on a later turn. The isolated UE 5.8.3 target compiles the changes,
and a bounded 15-minute `-NullRHI` Game pass subsequently verified the shared
tide cue and successful basin interactions. Rat's and the Innkeeper's saved
memories followed verified outcomes; Aster and Raven declined the optional
check-in and were left quiet. Runtime automation assertions and rendered
visuals remain outstanding. Details: [tide awareness and verified
interaction memories](findings/2026-11-01-tide-awareness-and-verified-memories.md).

## 2026-11-01 — Let seasonal flowers open and fade at different times

Named bluebell, marsh-marigold, and yellow-flag-iris instances now ease through
their bloom windows over an Island day, with deterministic local staggering so
each patch does not change all at once. This uses the existing 30-second
ecology refresh and ground-cover transforms; no plants, actors, collision, or
per-frame work were added. Closed blossoms are no longer chosen as pollinator
targets. The isolated UE 5.8.3 Editor target built successfully and the updated
`IslandWeather.SeasonalFlowers` automation passed. Its commandlet lingered after
the successful test because the early `Quit` was processed before the test
session; the bounded wrapper then stopped it. Rendered appearance, flower
readability, and play performance remain unverified while GPU headroom is below
the 4 GiB gate. Details: [seasonal flower transitions](findings/2026-11-01-seasonal-flower-transitions.md).

## 2026-11-01 — Let a ready basin interaction survive a recent inspection

The bounded 15-minute resident session caught Raven choosing to float a leaf
in a full basin from two metres away, but the controller rejected it because
the basin was still on its generic five-minute inspection cooldown. The basin
already enforces the meaningful limit itself: one leaf per resident per Island
day. Basin interactions now bypass only the generic inspection cooldown, while
ordinary landmarks retain five minutes and the reversible hearth retains one.
The UE 5.8.3 scratch Editor target compiled and linked the implementation and a
world-level regression for the exact sequence. Runtime automation remains
unverified because the known NullRHI bootstrap crash recurred before test
startup. Details: [basin interaction cooldown](findings/2026-11-01-rainbasin-interaction-cooldown.md).

## 2026-11-01 — Let residents recognize the seasonal flower patches

Now that dedicated woodland-bluebell, Tideglass marsh-marigold, and
yellow-flag-iris components have seasonal bloom windows, nearby residents name
those species in their visible-vegetation summary instead of receiving only
the generic "wildflowers" label. Existing visibility and line-of-sight rules
still apply, so dormant or hidden components are not described as present. The
worktree editor target built successfully; `NearbyVegetationPerception` and
`IslandWeather.SeasonalFlowers` each pass separately under `-NullRHI`, with
resident thinking disabled and maximum model requests set to zero (logs:
[`resident vegetation perception`](../Saved/Logs/Codex_SeasonalResidentPerception_20261101.log),
[`seasonal flowers`](../Saved/Logs/Codex_SeasonalFlowersFinal_20261101.log)).
The main D: project's module still needs this branch before residents there can
use the new labels.

## 2026-11-01 — Let habitat flowers follow the Island seasons

The persistent 90-day temperate calendar now drives a first small botanical
phenology pass: woodland bluebells and Tideglass marsh marigolds appear in
spring; yellow-flag iris appears in spring and summer. Their collisionless HISM
components switch only at the 30-second ecology refresh and on initial ground
cover setup. General meadow flowers stay year-round because their current
species identities do not yet have authored bloom windows. This adds no assets,
instances, or per-frame transform work; wildlife's nearest-bloom query naturally
stops treating dormant specialist flowers as present. The UE 5.8.3 editor target
built successfully with no-hot-reload flags, keeping the open editor untouched;
the `IslandWeather.SeasonalFlowers` automation passed under `-NullRHI` (log:
[`Codex_SeasonalFlowers_20261101.log`](../Saved/Logs/Codex_SeasonalFlowers_20261101.log)).
Runtime visual progression and gradual bud/flower/fade transitions remain
unverified and are the next polish step.

## 2026-11-01 — Let shore and woodland routines follow the changing day

The temperate calendar now changes sunrise and sunset, so shore animals should
not keep a fixed 06:00–19:00 activity window. Minnow rest eases around the
seasonal dawn and dusk while preserving its former equinox schedule. The
ecology scheduler now uses the same sunrise/sunset for fireflies, shore
crabs, dragonflies, minnows, and the day/night resting routines of woodland
residents. The rainbow's anti-solar direction also follows the season-adjusted
sun arc. This changes no population or actor budget. The UE 5.8.3 editor target
compiled and linked, including seasonal minnow and rainbow regression cases;
automation was not run because the known NullRHI startup issue persists.
Rendered seasonal behavior remains unverified while the play safety gate is
closed.

## 2026-11-01 — Give the Island's lunar clock a visible moon

The Island already tracked a 29.53-day lunar cycle, changing moonrise/set and
moonlight, but no moon appeared in the sky. The existing procedural sky mesh
now carries a small disc at the actual moon-light direction; it dims with phase
and cloud transmission, and hides below the horizon, in daylight, at new moon,
or under thick cloud. It uses 48 triangles and no additional actor or asset.
Build and deterministic visibility tests are the immediate checks; a night
render still needs GPU headroom. The first pass was a uniformly tinted disc.
A follow-up now shapes the actual lit silhouette through the waxing and waning
cycle, while leaving the unlit portion open so stars are not covered by a dark
circle. Details and unverified render limitations are in
[phase-shaped moon](findings/2026-11-01-phase-shaped-moon.md).

## 2026-11-01 — Let residents see the shooting stars they describe

The night calendar already made rare meteors part of the sky's narration, but
the procedural sky showed only fixed stars. An eligible clear-night slot now
animates one tiny, deterministic streak across the existing starfield, and the
resident observation is limited to that same brief crossing. It adds no new
actor or texture. The UE 5.8.3 scratch build succeeds; automation and rendered
appearance remain unverified because the NullRHI startup issue persists and GPU
headroom is only 258 MiB. Details: [visible shooting stars](findings/2026-11-01-visible-shooting-stars.md).

## 2026-11-01 — Let woodland communities vary without multiplying trees

Every grove previously shared identical planting radii, which can reveal the
procedural pattern even when individual positions are randomized. A smooth,
seeded habitat field now gently expands or contracts all five layers of each
grove while leaving population budgets and safety clearances intact. The change
builds successfully in the isolated UE 5.8.3 target; its rendered appearance
still needs a matched route comparison after GPU and system-memory headroom
meet the play gate. Details: [irregular grove footprints](findings/2026-11-01-irregular-grove-footprints.md).

## 2026-11-01 — Let a resident finish an interaction they explicitly chose

The isolated 15-minute feedback log caught Rat choosing the rain-basin leaf
interaction from roughly 17 metres away; the controller rejected it at the
4-metre boundary, so the basin's wet state never got a leaf. A bounded recovery
now lets a resident approach a visible stationary target within 25 metres after
that explicit choice, but does not approach wildlife or perform the interaction
until arrival is revalidated. Focused regression cases compile in the UE 5.8.3
worktree target; the known NullRHI startup crash and current GPU-headroom gate
mean the runtime pass is still pending. Details:
[bounded resident interaction approach](findings/2026-11-01-resident-interaction-approach.md).

## 2026-11-01 — Tighten the rendered-play safety gate after a PIE memory spike

The restarted D: editor reported 6,560 MiB of free GPU memory while idle, but
the first PIE preview fell to 483 MiB free and displayed Unreal's system
Memory Pressure Warning within 36 seconds. GPU free memory recovered only to
2,898 MiB after PIE stopped; Windows then had 1.90 GiB free physical RAM. The
session stopped with zero character-model requests. This is point-sample
evidence, not a frame-time or peak profile. It shows an idle-only 4 GiB GPU
gate is insufficient: check system RAM too, and stop immediately on the first
memory-pressure warning. Do not yet cut tree counts or disable Nanite based on
this alone. The live D: source also differs from the Codex worktree, so this
run does not validate the worktree's newer water or A/B canopy changes. The
bounded spectator launcher now has an opt-in Windows RAM preflight and
two-sample runtime monitor that complements the GPU guard; query failures fail
closed, and it only terminates its own launched Game process. The actual
low-RAM preflight rejected this machine at 2,269 MiB free. It does not intercept
manual PIE from the editor. Details:
[rendered PIE memory-pressure preflight](findings/2026-11-01-pie-memory-pressure-preflight.md).

## 2026-11-01 — Remove center-weighting from woodland scatter

Woodland layers already overlap, but their candidates used a radius drawn
uniformly rather than an area-uniform radius. That subtly packed candidates
toward each grove center. The seeded scatter now transforms the same single
random sample with the annulus area law for grove centers, mature trees,
saplings, shrubs, rhododendrons, and coppice. It preserves the current tree and
ground-cover budgets, spacing, and overlap. A focused math regression covers
the area median, endpoints, clamping, and invalid samples. This is a geometric
correction, not visual proof; the matched rendered comparison remains pending
while GPU memory is below the 4 GiB gate. Details:
[area-uniform woodland scatter](findings/2026-11-01-area-uniform-woodland-scatter.md).

## 2026-11-01 — Give each broadleaf canopy a second local silhouette

The Island's four broadleaf species now have a deterministic A/B mesh split,
using the installed Norway maple, European aspen, European beech, and black alder
variants. Variant assignment comes from an independent local field, so it is
stable for a given world seed and position and does not consume the scatter RNG.
The species counts, 3,600-tree broadleaf cap, existing cull distances, collision,
and navigation behavior are unchanged; unavailable B meshes or disabled/missing
DynamicWind data fall back to A. The new B components receive the same
per-species cull range and DynamicWind setup.

The UE 5.8.3 scratch Editor target compiled and linked successfully, and the
asset preflight found all eight A/B packages in the installed Content tree. The
offline AssetPipeline suite also passed 112 tests. This is implementation and
package-path evidence, not an in-game appearance, wind, memory, or performance
result: the live editor still has the previous DLL, whole-device GPU headroom
is 2,509 MiB (below the 4 GiB rendered-session gate), and the known NullRHI
pre-initialization crash prevents blindly repeating commandlet automation. The
woodland startup log now also reports A/B instance counts per species, so the
next run can verify how the valid-land scatter split actually materialized.
Keep this A/B split as the candidate for the matched woodland comparison; do
not increase population or extend cull distances. Full evidence and remaining
checks: [broadleaf A/B variant support](findings/2026-11-01-broadleaf-ab-variants.md).

## 2026-11-01 — Separate canopy counts from visitor-visible variety

The saved NullRHI spatial report confirms broad five-species canopy coverage,
but does not show what a visitor can distinguish. The live project source now
uses deterministic A/B variants for each broadleaf species, while keeping tree
counts and cull distances unchanged. Shorter broadleaf cull ranges than spruce
may still make distant views feel spruce-heavy; the A/B split's visible effect
has not been measured. An opt-in `-IslandBroadleafAOnly` launch profile now
forces the same deterministic placement field to use A meshes only, giving the
next view a direct, constant-population comparison. Keep cull distances fixed;
when GPU headroom allows, capture the same daylight route and record
close/middle/far silhouette readability, frame-time p95, and device VRAM. The
latest sample had 3,384 MiB free, below the 4 GiB render gate. Details and
exact counters:
[canopy coverage finding](findings/2026-11-01-canopy-variant-coverage.md).

## 2026-11-01 — Ground drift narration in visible motes

Resident situation summaries previously described airborne seeds, petals, or
leaves from local wind and rain values alone, despite the drift effect being
spawned around the local viewer. That could make a resident narrate something
they could not see. Residents now describe drift only when an active mote is
within 12 metres and, when a resident actor is available, has an unobstructed
visibility trace. The line names only the observed kind and remains empty when
there is no visible mote. Focused coverage checks nearby, distant, and inactive
leaf slots. This corrects perception grounding; it does not change mote
placement, weather, or the visual effect. The UE 5.8.3 editor target compiled
and linked successfully, including the focused regression cases. Automation was
not run because the known NullRHI startup crash has not been resolved. The
actual D: project reopened successfully and reports All Saved; no PIE session
was started because GPU headroom remains below the 4 GiB safety margin.

## 2026-11-01 — Confirm Nanite Foliage after the editor restart

The user-approved restart reopened the actual D: `CaptiveSky_2` project in UE
5.8.3. Startup waited several minutes for Zen Storage; the Zen log then
confirmed the server was running and the Island viewport loaded. Querying
`r.Nanite.Foliage` in the editor returned `1` with `LastSetBy:
ProjectSetting`. The editor reports **All Saved**. This confirms activation in
the live project, not a visual or performance improvement: the saved viewport
faces offshore and does not show runtime-generated tree communities.

The whole-device GPU sample was 9,475 / 12,282 MiB used (2,523 MiB free),
42% utilization and 68 °C. That is below the existing 4 GiB headroom gate, so
no PIE/Game run was started. Preserve the 30-FPS floor; next capture the
daylight Tideglass-to-woodland route with close/middle/far species visibility,
frame-time p95 and whole-device VRAM together, once headroom permits. The
landscape edit-layer merge warning remains visible and was not changed.

A later user-approved restart of the exact D: project completed at 07:56 UTC.
The fresh `CaptiveSky_2.log` again sets `r.Nanite.ProjectEnabled=1` and
`r.Nanite.Foliage=1`; Island MapCheck reports 0 errors and 0 warnings. At
08:01 UTC `nvidia-smi` showed 11,675 / 12,282 MiB used (323 MiB free), 47%
utilization, and 73 °C. No PIE or resident session was started. The latest
restart and resource gate are recorded in
[`editor restart memory recheck`](findings/2026-11-01-editor-restart-memory-recheck.md).
The restart had left two identical editor windows. Closing one left the Island
editor responsive and reduced whole-device GPU use to 8,451 MiB (3,547 MiB
free); this remains below the 4 GiB reserve, so rendered play is still deferred.

## 2026-11-01 — Guard the balance of the broadleaf habitat field

The 81-point representative Island-scale broadleaf test previously selected
the four species 12, 28, 26, and 15 times. It now also rejects a selection
field where any species falls below 8 or exceeds 32 samples, protecting
meaningful representation without pinning exact counts. This is a seeded
coordinate-field regression only: it does not apply the valid-land mask or
prove visible runtime placement. The UE 5.8 editor target compiled and linked
successfully, including `IslandWeatherBroadleafTests.cpp`. The automation was
not run; the known worktree NullRHI pre-initialization crash remains the reason
not to retry that launch blindly. Next visual confirmation stays the matched
daylight Tideglass-to-woodland route, gated on at least 4 GiB free GPU memory
and the existing 30-FPS floor.

## 2026-11-01 — Let residents pass the Listening Stones' quiet practice along

A resident who directly witnesses another's quiet visit can now optionally
share that practice in an ordinary nearby resident conversation using
`teach_practice: "quiet-listening"`. The speaker must already hold the
witnessed practice memory; the recipient gains one private, attributed memory
only after the line is delivered through a reserved in-world conversation.
Invented IDs, uninformed speakers, player/Discord exchanges, and out-of-range
lines cannot grant the practice. This adds a small path for place etiquette to
spread without a global culture score, extra model call, or mandatory teaching.

The focused `Agent.CraftTeaching` regression now covers the shared
conversation transport, exact optional ID, durable memory reload and
attribution, activation of the existing non-interruption choice, speaker-
knowledge gate, invented IDs, and range gate. The UE 5.8
editor target builds successfully, but the automation test has not been run:
the worktree's headless NullRHI startup previously crashed before harness
initialization, and the separate live D: project has not loaded this worktree's
module. Keep the
runtime transmission and ordinary resident pacing unverified until a safe
editor test session is available.

## 2026-10-31 — Surface missing-object requests in the optional resident check-in

The twelve-minute resident check-in already invited optional `request_upgrade`
proposals, but did not mention the separate `request_object` path. It now lets
a resident suggest one concrete physical object if something useful feels
genuinely missing. Both paths remain optional human-review proposals: the
check-in does not build, generate, import, or place anything, and residents may
ignore it. The UE 5.8 build succeeded, and the focused `Agent.ModelTier` and
`Agent.ModelTierWorldFacts` automations passed. This improves the invitation
contract, but does not establish whether residents make useful requests
without feeling prompted into a duty.

**Bounded feedback attempt:** after rebuilding the scratch-project module, the
900-second isolated NullRHI feedback run exited before the Island's world-ready
marker. Its crash context records a null-address access violation during engine
pre-initialization (`PCallStackHash 4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`);
there is no resident session or model-request result to report. The launcher
correctly rejected the zero exit code as startup failure. The run's data root
was isolated under `Saved/CS58/Saved/Playtests/Codex_ObjectFeedback_20261031/`,
so shared Island state and the normal proposal inbox were untouched. Do not
repeat the same launch blindly; obtain symbolized dump analysis or a fresh
startup diagnostic before the next feedback attempt.

**Follow-up startup diagnosis (22:42 UTC):** the 64-request feedback launch
and a fresh 32-request retry both exited before the world-ready marker; their
data roots were separate and no resident session started. A 30-second,
zero-request `UnrealEditor-Cmd.exe` NullRHI smoke run also failed at startup
with Trace Server launch disabled. Its crash context records the same kind of
null-address access violation on the game thread, with only UnrealEditor-Core
frames and `PCallStackHash F405610C114D120D4B6F5ABA901EC469AC9FAA26` (0 seconds
since start). The captured context/minidump was overwritten by Unreal's reused
`Saved/Crashes/_0000` slot; individual launch logs remain there. No local CDB,
WinDbg, or Unreal Core PDB was found, and Windows Application event-log reads
were denied. This makes the Trace Server error and request cap insufficient
explanations, but does not identify the fault. Stop further blind retries until
symbolized dump analysis or a materially different startup path is available.

## 2026-10-31 — Verify Nanite Foliage after restarting the actual project

After the user confirmed the Island was saved and approved a restart, the active
UE 5.8.3 editor was closed and reopened through Unreal's project browser. The
selected recent-project entry identified
`D:\Projects - Athena\Unreal\CaptiveSky_2\CaptiveSky_2.uproject`, not the
Claude-named sibling or this Codex worktree. The new D: startup log records
`r.Nanite.Foliage=1` from the project setting, and the viewport returned to the
saved Island. The visible post-load Output Log tail had no Nanite-disabled
assembly warning; it did show the existing landscape edit-layer merge batch
resolution warning. This verifies activation in the correct running editor,
not tree appearance, frame time, or an improvement in VRAM.

The editor's viewport briefly reported that it was waiting for Landscape
texture resources. Once the project had loaded, `nvidia-smi` still showed only
2,083 MiB free (9,915 / 12,282 MiB used), 75 °C, and 59% GPU utilization.
That remains below the guarded 4 GiB headroom needed for rendered play, so no
PIE/Game session was started. The 30-FPS floor and 15–30 minute feedback run
remain pending. No D: project code, content, or maps were edited during this
verification.

An editor-only establishing screenshot was captured at
`D:\Projects - Athena\Unreal\CaptiveSky_2\Saved\Screenshots\WindowsEditor\HighResScreenshot00003.png`
(1280×720). It shows the distant Island, bright cloud cover, and calm water
reflection; it is a useful mood/lighting reference, not a gameplay screenshot.
The edit-time viewport does not show the runtime-generated tree communities,
so it cannot settle the reported species-silhouette or natural-scatter issue.
The latest device check is still below the rendered-play gate: 9,917 MiB used,
2,081 MiB free, 50% GPU utilization, 78 °C. Keep the fixed tree budget and
species mix unchanged until a runtime view can compare placement and silhouette
above the 30-FPS floor; then run the bounded 15–30 minute character-feedback
session with live memory and frame-time monitoring.

## 2026-10-31 — Give the Island's quiet turns more room to breathe

The 15m52s standalone preview recorded 45 resident decisions and 49 model
requests. The controller previously allowed each resident's autonomous
background turn every 60 seconds, which makes a 30-minute session likely to
reach the existing 64-request ceiling early. The background interval now has a
120-second minimum, including legacy Blueprint values; repeated no-progress
choices still back off to at most five minutes. Visitor and resident-to-resident
conversations are not delayed by this pacing rule. The existing request ceiling
is unchanged. The focused `CaptiveSky2.Agent.SessionSafety` automation now
passes with the new timings. This is not yet a measured 30-minute runtime
result; a later guarded session must confirm the real request rate.

After the cadence rebuild, a headless NullRHI regression pass also reconfirmed
`Agent.ModelTier`, `Agent.ModelTierWorldFacts`,
`Agent.NearbyVegetationPerception`, and
`Agent.IslandWeather.BroadleafHabitatSelection` (all passed; zero model
requests). Logs: `Saved/Logs/Codex_EcologyFeedbackRegression_20261031.log`,
`Saved/Logs/Codex_CaptiveSky2_Agent_NearbyVegetationPerception_20261031.log`,
and `Saved/Logs/Codex_CaptiveSky2_Agent_IslandWeather_BroadleafHabitatSelection_20261031.log`.
These are source-level checks, not a rendered Island or character-response
playtest.

## 2026-10-31 — Collect resident reflections without adding render load

The spectator launcher now has an explicit `-NullRHICharacterFeedback` mode
for isolated, hidden character sessions. It requires situation logging, Python
disabled, a separate `-DataRoot`, and explicit caps; it is limited to 900 real
seconds and the existing 64-request hard ceiling. Ordinary NullRHI diagnostics
still require thinking disabled. This lets the check-in be observed while
rendered play is below the 4 GiB GPU-headroom threshold.

The first run used a 32-request cap and ended safely after 776 seconds once the
budget drained. Only Rat and the Innkeeper received the check-in; both chose
silence. A second run with the unchanged 64 hard cap reached its full 900
seconds, used 30 requests, and ended with none in flight. All four residents
received the optional check-in: Rat shared, “Good evening. The rainbow over
the water is a lovely sight after the rain.” Aster, Raven, and the Innkeeper
chose no spoken response and continued their own activities. This validates
the bounded feedback path and preserves resident choice; it is not a rendered
visual, FPS, VRAM, or shoreline-collision test. Both logs and isolated world
states are under `Saved/CS58/Saved/`.

The editor target built against the scratch project using worktree source and
a junction to the main project's Content. The main map/assets were not edited.
The new launcher guard tests passed (five tests), the PowerShell script parsed,
and a 65-request attempt was rejected by the 64 limit. The implementation is
pushed; main-project sync still awaits explicit authorization.

## 2026-10-31 — Make optional world feedback actionable without making it a duty

The delayed resident check-in now names the existing `request_upgrade` action
and its three proposal kinds (aesthetic, variation, functionality), so a
resident can connect an observed object or place to a concrete idea. The prompt
still allows silence, does not ask the resident to build, and makes clear that
all proposals require human review and change nothing automatically. The action
is discoverable, not forced or repeated; no additional model request was added.
UE 5.8.3 rebuilt the worktree editor target successfully, and the focused
`ModelTier` plus `ModelTierWorldFacts` tests both passed under NullRHI with
resident thinking disabled and zero model requests. This verifies the prompt
contract, not that residents have yet made useful proposals in a 15-minute
world session; that remains a guarded playtest task after the worktree behavior
is integrated into the main project.

## 2026-10-31 — Make the delayed resident-feedback invitation recoverable

The optional check-in becomes eligible at 720 real seconds on an autonomous turn,
never interrupts visitor or agent conversation, and is sent at most twice per
resident per session: the initial invitation plus one retry if the provider
fails or returns an invalid decision. A valid response with no speech is treated
as an intentional pass and is not retried. This preserves the character's
choice while preventing a transient provider error from consuming the only
chance to receive feedback.

The UE 5.8.3 editor target rebuilt successfully with this retry logic, and a
headless `-NullRHI` run passed both `CaptiveSky2.Agent.ModelTier` and
`CaptiveSky2.Agent.ModelTierWorldFacts` (`Result={Success}`, exit code 0;
[`Codex_ResidentFeedback_ModelTier_Retry20261031.log`](../Saved/Logs/Codex_ResidentFeedback_ModelTier_Retry20261031.log)). This is source-level eligibility and retry-bound evidence, not a 12-minute resident response or a 15–30 minute world playtest. The sparse worktree lacks `Content/Maps/Island.umap` and local plant assets, so it cannot validate the complete Island or runtime visual quality. The retry logic and lower-only playtest cap remain in the worktree; the open D: project source has not been synchronized. Next, obtain approval to sync/rebuild those safeguards in D:, then run a bounded character-feedback playtest and record the response, frame rate, and device-memory state.

The retry predicate is now a separately tested pure helper: provider failure and
invalid decisions may consume the one remaining retry, while a valid response
without speech is a deliberate pass and cannot retry. Negative attempt counts
and requests beyond the two-attempt ceiling are also rejected. The expanded
`ModelTier` and `ModelTierWorldFacts` tests both passed after a fresh rebuild
(`Saved/Logs/Codex_ResidentFeedbackRetryPolicy_20261031.log`, exit code 0).

## 2026-10-31 — Recheck the RatHollow V5 candidate

The local V5 RatHollow FBX rebuilt and re-imported cleanly in Blender 5.2.2
(2,352 triangles; five box colliders; `UVMap`; `PROBLEMS []`), and 25
RatHollow-related source tests passed under Blender's bundled Python. The
entrance-facing preview is readable but still looks like a short, manufactured
stump with dominant high-contrast bark; it is not ready for placement. A
read-only check of the actual D: project found older RatHollow imports but no
V5 asset and no canonical `wildlife_homes` record. Nothing was imported,
placed, or saved. The user's chosen hollow concept remains unresolved pending a
disposable Island review of ground contact, concealment, and Rat-scale
navigation. Full assessment: [RatHollow V5 recheck](findings/2026-10-31-rathollow-v5-recheck.md).

## 2026-10-31 — Compare a lower-contrast RatHollow bark material

The separate V6 RatHollow candidate reuses the V5 shell with a generated,
versioned, darker broadleaf-bark albedo; it leaves the shared V4 maps alone.
Blender export/re-import verification passes (2,352 triangles, five simple
colliders, `UVMap`, `PROBLEMS []`), as do the new texture-pack regression and
25 existing RatHollow tests. Its entrance-facing neutral preview is warmer than
V5, but it still reads as a stylized stump, and the reused V4 normal map keeps
the bark too directional. Do not import or place it until fresh PBR derivatives
and a disposable Unreal woodland review establish a better result. Full prompt,
verification, and limits: [RatHollow V6 bark candidate](findings/2026-10-31-rathollow-v6-bark-candidate.md).

## 2026-10-31 — Stop the first live PIE memory sample early

The currently open `CaptiveSky_2` editor reported `r.Nanite.Foliage = 1`
(`LastSetBy: ProjectSetting`) in its console. A PIE Island session did start and
the live log showed resident decisions for Raven, Aster, the Innkeeper, and Rat,
but the viewport immediately displayed: “Video memory has been exhausted
(184,840 MB over budget). Expect extremely poor performance.” PIE was stopped
after 32 seconds rather than leaving it to run under that warning. The session
log reported four completed model requests and zero still in flight; it also
warned that it could not find a `RecastNavMesh` instance while creating a crowd
manager. The same Output Log showed `M_Island_Textured_Wet` requesting invalid
texture index `21 / 20`; the current worktree has no matching `Content` asset
to inspect, so the warning's relationship to the active project material is
unconfirmed. This is a short runtime observation, not a 15–30 minute playtest or
proof of the user-requested feedback check-in. The warning's displayed numeric
value is preserved verbatim, not treated as a trustworthy measurement. Before
another long run, identify the source of the memory warning and confirm the
editor project copy and device-level budget; retain the 30-FPS floor and
request/deadline safeguards. Immediately after PIE stopped, `nvidia-smi`
reported 8,600 MiB used of 12,282 MiB on the RTX 4080 Laptop GPU, but WDDM
provided no per-process breakdown. This post-stop snapshot does not establish
which process or resource caused the in-PIE warning. A read-only editor
`stat Streaming` sample then showed a 3,693.27 / 4,096.03 MB streaming pool
(90%), with 375.92 MB non-streaming mips and 281.59 MB wanted mips. `stat RHI`
separately reported 2,966.44 MB of Render Target 2D memory and 1,874.67 MB of
UAV texture memory; these RHI categories are not assumed to be additive. The
pool is already near its configured ceiling in the editor, but these samples
were taken after PIE and do not prove the immediate warning's cause. Epic's
[texture-streaming reference](https://dev.epicgames.com/documentation/unreal-engine/texture-streaming-configuration-in-unreal-engine)
defines the pool and the VRAM-limit setting; don't increase the 4,096 MB pool
without a measured device-memory budget. The same editor log reports 900 each
of Norway maple, European aspen, European beech, and black alder instances, plus
39 birch and 57 hazel coppice instances. That confirms the active generator has
four tree species; their distinction is not obvious in this particular view.
The next visual pass should inspect species silhouette, material separation,
and habitat placement before changing the fixed tree budget.

**Rendered-session headroom gate (2026-10-31):** while the restarted editor was
open, `nvidia-smi` reported 9,888 / 12,282 MiB used (2,394 MiB free), 78 °C,
and 55% GPU utilization; WDDM exposed no per-process breakdown. No PIE/Game run
was started. The worktree spectator launcher now offers an opt-in
`-RequireGpuHeadroom` check that requires at least 4 GiB free before starting a
rendered session. Its live check correctly refused the present 2,394 MiB
snapshot without launching Unreal. This is only a device-wide preflight; it
does not prove runtime headroom or replace in-run FPS and memory monitoring.
Details and tests: [GPU memory preflight](findings/2026-10-31-gpu-memory-preflight.md).

## 2026-10-31 — Make nearby object proposals pointable by name

The request-upgrade prompt already allowed residents to propose aesthetic,
variation, or functionality changes to any landscape/level feature. Nearby
ordinary props can also be surfaced, but an engine-generated actor label such
as `StaticMeshActor_27` is not a useful thing for a resident or human reviewer
to identify. When a prop has only a default generated label, perception now
falls back to its mesh asset name (for example, `SM_Notice_Board_Weathered`
becomes “Notice Board Weathered”). Authored actor labels remain preferred.
This changes only what is named in the situation summary: it neither grants a
movement/interaction target nor edits the object. A focused regression checks
that an inspectable nearby notice board is targetable in a human-readable
upgrade proposal and that the generated actor label stays hidden. The UE 5.8.3
editor target built, and `CaptiveSky2.Agent.NearbyVegetationPerception` passed
under NullRHI (exit code 0); its warmed 3,600-tree fixture averaged 0.198 ms
across ten samples. This does not establish live prop labeling or Island visual
quality in the active D: project.

## 2026-10-31 — Let residents notice the mixed broadleaf canopy

The existing nearby-plant perception already described visible HISM ground
cover, but the Island's Norway maple, European aspen, European beech and black
alder groves use `UInstancedSkinnedMeshComponent` and were invisible to that
scan. `BuildSituationSummary` now also checks those instanced-skinned patches
within 6.5 m, sorts local transforms by distance, and performs visibility traces
for at most the three closest candidates per component. It uses ordinary
species names and frames the patch as optional `request_upgrade` context only;
plants remain without movement or interaction targets. This is descriptive
perception only: no extra model call, world mutation, memory write, or foliage
population increase. The summary remains capped at three plant patches; when
nearby HISM ground cover would otherwise occupy all three slots, it reserves
the third for the nearest visible broadleaf patch while keeping the two nearest
other patches. A crowding regression covers this mixed-canopy case.

The UE 5.8.3 editor target built successfully, and
`CaptiveSky2.Agent.NearbyVegetationPerception` passed under NullRHI (exit code
0). The fixture verifies the Norway-maple label, keeps it out of movement-target
wording, and omits it when hidden, occluded, or outside the short radius. A
crowding case places three closer HISM patches before the maple and verifies
that the two closest plus the maple—not the third HISM—fit the three-patch cap.
A stress fixture populated all four species components to the 3,600-tree
budget and timed ten warmed summary builds at an average of 0.132 ms under
NullRHI. This isolates CPU transform scanning with transient meshes; it is not
a full Island prompt-construction profile and does not validate live species
visibility or visual quality. Keep the 6.5 m radius and bounded visibility
traces until an in-world daylight check confirms what residents actually see.

**Active-project setting check (2026-10-31 14:20):** the actual
`D:\Projects - Athena\unreal\captivesky_2_claude\Config\DefaultEngine.ini`
sets `r.Nanite.Foliage=False`; that project's latest editor log (14:01 startup)
sets the CVar to `0`, and its three preceding 2026-10-31 backup logs do too.
This branch's separate worktree config says `True`, but it is not the project
loaded by the user's editor. The earlier console reading of `1` belongs to a
different preview/context and does not prove the D: project has Nanite Foliage
enabled. The approved restart was not carried out: the desktop bridge could
capture the window but would not send it input. The D: project is outside this
worktree's writable scope, so no setting was changed. Enable it in that project
once that write is authorized, restart, and confirm the startup log before
treating Nanite Assembly warnings as resolved.

## 2026-10-31 — Let an exposed roost become a character-led proposal

Raven's latest isolated playtest again found the west roost exposed to weather,
then chose to rest there. When the existing five-point overhead-cover check
finds no hits, Raven's read-only roost assessment now offers an optional,
exact-target `request_upgrade` suggestion for a sheltered nest/perch. It is
explicitly a human-reviewed proposal: it changes nothing, does not promise a
weatherproof result, and is not a duty. This connects a lived observation to
the existing resident-request pipeline without adding another model call or
automatic construction.

The UE 5.8.3 worktree editor target built successfully. The new assertion in
`CaptiveSky2.Agent.RavenPerch` passed in a NullRHI run, but the full automation
still fails later because this sparse worktree lacks the Raven placeholder
Blueprint and several referenced Content assets. Rerun the complete test when
the project Content is available. The regression also confirms that a site
with all five overhead probes covered does not trigger the proposal cue. The
open D: editor and map were not changed.

## 2026-10-29 — Make the existing diversity visible before adding more

The current generator already places a fixed 3,600-tree broadleaf mix across
hundreds of groves, uses seeded species communities and varied canopy heights,
and spreads the low-cover budget beyond landmark rings. The latest runtime
preview still read as a few similar dark silhouettes at night; plant-material
fallback warnings and very short tree cull distances can conceal variety that
the counts prove exists. Several graph-valid coppice materials have since had
their instanced usage repaired, but the daylight effect and graphics-memory
cost remain unmeasured. Do not raise global vegetation counts to solve a
readability problem.

**Cull-horizon diagnosis (2026-10-31):** the current constructor gives Norway
maple, European aspen, and European beech an 80–125 m fade/cull range; black
alder reaches 120–220 m, while spruce reaches 350–550 m. This is a concrete
reason distant views can collapse to “one kind of tree” even when the local
grove mix contains four broadleaf species. It does not prove that culling is
the only cause: fallback materials, species placement, and the selected view
still matter. Do not extend the three broadleaf horizons until a matched
daylight route measures species readability, frame-time p95, and whole-device
VRAM; a far-canopy proxy could later retain silhouette variety at lower cost.

### 1. Finish one daylight coastal-to-woodland visitor route

Use the current fixed-budget meadow and mixed-grove placement as the baseline.
Capture one short daylight traversal from the Tideglass edge into a broadleaf
grove, with resident thinking disabled and the existing realtime/request caps
left active. Record the same route's species visibility, frame-time p95 and
whole-device graphics memory; preserve the 30-FPS floor and stop if memory
pressure appears. Compare the distinctive tree silhouettes at close, middle
and far distance instead of inferring naturalness from instance totals.

An earlier standalone preview read `r.Nanite.Foliage=1` after restart; the later
active-project check above shows this was not the D: project currently used by
the editor. The Island ocean automation passes, including disabling legacy
ocean-plane collision and transitioning grounded characters into swimming.
This verifies the movement/collision path, not the visual swim animation; the
character animation graph still has no swim state, so keep the water-walking
report open until a short visual test confirms the character's position and
pose at the surface.

The broadleaf diversity automation currently fails only its two alder cull
distance assertions in the live editor: its compiled test expects 80/125 m,
while the loaded component reports 120/220 m. The checked-in source and test
both specify 120/220 m, so this points to stale editor binaries. Rebuild from
the current source before treating that test as a code regression. The
BroadleafMixing automation passed. The next useful visual pass is still one
bounded daylight traversal from Tideglass to a broadleaf grove, with per-species
visibility checked close, middle, and far; do not increase foliage counts to
compensate for a visibility or material issue.

**Meadow acceptance regression correction (2026-10-31):** an earlier change
intentionally broadened landscape acceptance from 40–64% to 28–100% while
preserving the same 46% mean, creating stronger open glades and lush patches
without increasing the candidate budget. Two endpoint assertions still
expected the old range; they now match the current function. UE 5.8.3 rebuilt
the isolated worktree target, and the exact `CaptiveSky2.Agent.IslandWeather`
automation passed under NullRHI (exit code 0, no resident model requests).
This validates the scalar habitat rule, not the resulting Island scatter,
visual naturalness, frame rate, or VRAM. The broader prefix run separately
passed habitat-selection and height-profile tests; its diversity test could
not complete because this worktree lacks the Black Alder asset.

**Representative habitat-grid regression (2026-10-31):** the broadleaf test
no longer probes a one-dimensional line extending 10 km off-map. It now checks
all four species against an 81-point, 3 km coordinate grid around the authored
Island center, using the same seed salt as tree placement. The worktree target
compiled successfully. A single-precision reproduction of the field selects
the four species 12, 28, 26, and 15 times respectively. This checks field
coverage only; it does not apply the landscape validity mask or prove visible
trees. UE commandlet attempts using both the CS58 scratch project and the root
worktree crashed before automation discovery with an access violation, leaving
the actual C++ test result unverified (`Saved/CS58/Saved/Crashes/_0000/` and
`Saved/Crashes/_0000/`). Do not treat the arithmetic reproduction as a test
pass; rerun the automation once commandlet startup is stable.

**Fresh viewport check (2026-10-31):** the restarted editor is responsive and
the daylight viewport is unobstructed. In the current wide view, foreground
groundcover forms separated clumps, but the broad slope reads mostly bare and
only a few tree silhouettes stand out. This angle cannot establish which tree
species are visible, and no map or runtime state was changed. Keep the next
check at the woodland transition: identify the species at close/middle/far
distance and record p95 frame time plus whole-device VRAM in the same single-
editor run. Historical captures vary from 18.03 ms p95 in a stationary view to
23.84 FPS p95 for a full-cover capture, so a density increase is not justified
without a matched measurement.

**Bounded runtime recheck (2026-10-31):** after the approved editor restart,
`r.Nanite.Foliage=1` remained active during a 15m 52s standalone preview
(49 model requests; isolated playtest data, according to the Unreal session
log). The night view around the rain basin showed dense layered understory and multiple tree silhouettes, but this
local composition does not establish natural scatter or species readability
over the island; the editor-only view still cannot show the runtime-generated
broadleaf population. Raven again signaled that the west roost is too exposed,
Rat preferred a sheltered hollow, and Aster floated another leaf in the basin.
The run was not a performance profile and did not test water walking. Next:
capture a matched daylight woodland-transition traversal with close/middle/far
species identification, frame-time p95, and whole-device VRAM; separately test
ocean contact and swim pose. See the [playtest record](findings/2026-10-31-character-playtest.md#second-bounded-preview).

**Request-cap mismatch (2026-10-31):** the newly reopened local project copy
reported a default 1,800-second / 120-request play cap, unlike the earlier
64-request preview. A dark PIE attempt was stopped after 82.8 seconds and nine
requests, before any water-contact validation. In this tracked worktree the
configured default and hard clamp are now both 64; the focused UE 5.8.3 build
and `CaptiveSky2.Agent.SessionSafety` automation pass. Do not start another
long model-active session until the intended project copy is rebuilt and its
log confirms the 64-request ceiling.

### 2. Repair presentation dependencies in bounded groups

Resolve the `M_Island_Textured_Wet` texture-collection index warning separately
from GPU-memory measurements. Audit remaining instanced-plant material roots
before enabling usage; the Rhododendron master has unresolved material-function
calls and must not be toggled or reconstructed blindly. After safe material
repairs, verify that authored foliage materials render in the same daylight
route. Treat texture-pool size, transient render targets, whole-device VRAM and
frame time as different measurements.

### 3. Evaluate a two-distance canopy only after the baseline

UE 5.8 Procedural Vegetation Editor can emit Nanite-ready static meshes as well
as wind-capable skeletal meshes. A paired near-wind / far-static representation
could add readable middle-distance canopy without multiplying instance count,
but the current Megaplants are single-LOD and the representation swap has not
been proven. First make an isolated PVE copy, compare its outputs and texture
dependencies, and review the visual transition and memory cost before changing
the Island population.

## 2026-10-25 — From scattered systems to legible world structure

The latest repository work changes the recommendation set: broadland cover is reduced and clustered; maple/aspen/beech communities now substitute into the existing grove/tree budget; Nanite foliage is enabled but VRAM remains constrained; surface swimming/wake logic has focused regression coverage after fixing a false shore hit against another pawn; Listening Stones gained visitor rules; and the old floating PCG forest is not active in the current Island.

### 1. Turn the trail ledger into the first canonical desire path

**Player experience:** repeated travel slowly becomes visible: vegetation thins, ground compacts, and a narrow path emerges. If traffic stops, recovery can later begin. The path is evidence of behavior rather than an authored spline pretending history happened.

**Why it matters:** this directly connects agency to physical history and gives later settlement/construction a precursor: circulation appears before infrastructure.

**Why it fits now:** the repo already has an Island trail ledger with finite-input validation, runtime scatter, habitat clustering, clearance logic, and repeatable visual/performance review.

**Smallest practical step:** aggregate valid footfall/trail evidence into one canonical-friendly desire-path record: stable path/region ID, bounded polyline or cells, accumulated use, last-use day, recovery state. Initially materialize only presentation: suppress ground-cover candidates inside a narrow falloff and blend compacted-ground/wetness. Do not deform base geology or let an LLM author raw splines.

**First visual milestone:** the same 20–30 m corridor at low, medium and high use: intact meadow → faint broken line → unmistakable narrow path with soft vegetation edges, reversible by resetting the record. A strong compacted-soil/stone/leaf-litter material blend would have disproportionate impact.

### 2. Make one storm-quality coastal slice instead of another foliage-density pass

**Player experience:** Tideglass/open coast finally demonstrates why Unreal is the premier lens: wind-driven vegetation, wet materials, cloud-softened light, rain over water, spray/mist, readable broadleaf/conifer masses, and the player entering the ocean with a restrained wake.

**Why it matters:** this is the primary visible payoff. Recent repo evidence shows that large tree/ground-cover populations can still read sparse, while PIE has approached the GPU's VRAM ceiling. More instances are not the answer; composition, material response, culling/streaming and atmospheric depth are.

**Why it fits now:** mixed spruce/maple/aspen/beech groves, clustered cover, rain/cloud/moon/wind ecology, ocean water, surface swimming and procedural wake already exist. The PCG audit says the old floating forest is not active, so current runtime scatter is the right thing to art-direct.

**Smallest practical step:** choose one coastal traversal from sheltered land into open water as a bounded beauty/performance slice. Author silhouette hierarchy first; aggressively cap/stream expensive single-LOD Megaplants; repair the current wet-landscape texture-index warning; then tune storm sky, wetness, water and near-camera vegetation. Capture GPU allocation/streaming and p95 in the same session.

**First visual milestone:** a 20–30 second player-controlled sequence under a clearing storm: mixed woodland edge → desire-path opening → wet rock descent → clean ocean entry → swimming with subtle wake, with no VRAM warning and p95 above the existing 30-FPS floor. Excellent wet-rock surfaces, one low-cost broadleaf canopy family and convincing rain/spray VFX would matter more than more species.

### 3. Let the Listening Stones become a place with remembered etiquette

**Player experience:** residents learn how others use the Stones: sit quietly, leave an arrangement, avoid interrupting somebody already listening, or later tell another resident that people treat the place carefully. Repeated behavior can become convention without a global CultureScore or hard-coded sacred-site flag.

**Why it matters:** culture becomes shared expectation. Existing art/arrangements and visitor behavior can become a precursor to institutions: residents recognize, imitate, teach and preserve a norm tied to a place.

**Why it fits now:** persistent Listening Stone presentation/weathering, resident-made arrangements, memory/conversation machinery and fresh visitor rules already provide the substrate.

**Smallest practical step:** persist one renderer-independent place-practice evidence record when a resident directly witnesses meaningful Stone behavior: observer, place, observed action/motif, actor if known, day and confidence. Let existing scheduled decisions optionally reference learned practice; no background model calls and no omniscient attribution.

**First visual milestone:** blue hour after rain: one resident occupies the Stones while another approaches but chooses a respectful edge position; an older weathered arrangement and a newer derivative are visible. The scene communicates that people have learned how to behave here without UI text.

## Concept art — Wild card: The storm elevator

This week deliberately leaves the recurring golden-hour Tideglass terrace composition. This is a long-horizon possibility-space target: a high, rain-lashed cliff refuge reached by the cross-world elevator. It is not a proposal to build this location next week.

**Direction / reproducible prompt:** Wide cinematic 16:9, eye-level from a wet exterior landing cut into a sheer coastal cliff during a violent clearing storm near sunset. The left two-thirds open onto enormous geography: black-blue sea hundreds of metres below, fractured islands and sea arches, waterfalls blown sideways from forested shelves, low cloud and mist racing through ravines, a narrow break in the storm revealing cold gold light on the horizon. Foreground rain strikes dark local stone and runs through engineered drainage channels; restrained path lights reflect in puddles. At center-right, an elevator shaft is integrated vertically into the cliff with precise glass/metal doors and exposed rock kept visible. The elevator has just opened from another Captive Sky world. One rain-soaked resident and a dog stand at the threshold looking outward; they carry practical contemporary gear appropriate to the weather, not costumes. Behind the doors is a warm, compact social refuge/bar carved partly into the rock: timber where tactile, metal/glass where structurally useful, stone left natural, plants only where the microclimate supports them. A few residents inside talk quietly; one wall holds small objects/artifacts brought from other worlds, making cross-world cultural exchange visible without screens or exposition. No neon cyberpunk, medieval fantasy, luxury-resort excess or generic spaceship language. Technology feels maintainable and purposeful. Strong warm/cool contrast: amber interior/elevator light against slate storm, cyan-gray water and wet rock. Volumetric rain/mist, spray, moving foliage, waterfall turbulence and reflective surfaces provide spectacle. Mood: sublime, slightly dangerous, humane, exploratory. Signature detail: through the open elevator, a sliver of impossible bright white from the previous destination remains visible behind the warm cabin, quietly implying that the doors connect realities rather than floors.

**Generated concept image:** generated during this weekly run and delivered in ChatGPT. The connected GitHub content writer accepts UTF-8 text rather than generated binary image bytes, so no repository image path is claimed.


## Concept-art exploration rule — 2026-10-18

Weekly concept art should explore Captive Sky's possibility space rather than converge on a recurring house style. Avoid repeatedly defaulting to golden-hour Tideglass/coastal overlooks, terraces, craft tables, and idyllic vegetation compositions even when those are attractive.

Vary location, scale, weather, season, time of day, camera language, degree of habitation, emotional register, technology, and the part of the simulation being visualized. Beauty need not always mean idyllic. Concepts may include storms, interiors, unusual viewpoints, wildlife-scale scenes, aerial/historical views, night gatherings, harsh or empty landscapes, future settlements, parallel worlds, the elevator, the endless white plane, the ocean stairway, or different levels of simulation.

Do not reuse the previous four weekly concepts' primary location + composition + time-of-day combination. At least once every 3–4 weekly concepts, make the image a long-horizon **wild card**: adventurous concept art that explores what Captive Sky might eventually contain even when it is not the most immediately buildable recommendation that week. Weekly build recommendations should remain grounded in the current repository; the concept image is allowed to reach much farther.

## 2026-10-13 — Give the dragonfly's flower pause time to read

The existing flower-interest cue was only 2.6 seconds end-to-end, leaving about
1.25 seconds at its strongest approach/hover alpha. It now lasts 4.8 seconds:
the dragonfly eases in, holds its flowerward attention for roughly 3.1 seconds,
then fades back to its ordinary patrol. The 9 m search radius and 16-second
per-insect cooldown are unchanged; rain, ripple, and nearby-observer
interruptions still take priority, and flower instances remain read-only.
UE 5.8.3 built successfully and `CaptiveSky2.Agent.NightEcology` passed under
NullRHI with agent thinking and world-state persistence disabled
(`Saved/Logs/Codex_DragonflyBloomPause_20261013_Retry.log`). A bounded 90-second
daylight Game pass then loaded Tideglass, made zero model requests, and ended
normally. Fifteen fixed-view frames at five-second intervals show dragonflies
crossing the open view, but none clearly shows the flowerward hover
(`Saved/Logs/Codex_DragonflyBloomPause_Tideglass_20261013.log`; local captures
in `Saved/Playtests/Codex_DragonflyBloomPause_20261013/Screenshots_Tideglass/`).
That is not proof the behavior failed: the 4.8-second cue can begin and end
between five-second samples. The timing and interruption contract is verified,
but player legibility remains unproven. Next, use a close, bloom-level view and
one-second sampling (or temporary event telemetry) under the same zero-request
cap; only then decide whether to tune the cue or its opportunities. Two attempts
to tighten the sampling to one second instead crashed during Unreal startup,
before the Island became ready; one retry omitted the explicit DDC overrides,
so those switches alone do not explain it. The latest crash context reports a
null-pointer access violation at process startup, but has no symbolized stack
and the requested game log was never created
(`Saved/Crashes/_0000/Codex_DragonflyBloomPause_CadenceRetry_20261013.log`,
`Saved/Crashes/_0000/CrashContext.runtime-xml`). Stop repeating the same launch;
diagnose that startup crash before resuming the visual test. After retiring the
disabled PlantFactory mount, one 30-second, zero-request retry with the former
five-second cadence and warmed project-local DDC failed the same way. Its crash
context again reports a null-address access violation at `SecondsSinceStart=0`
and the same `PCallStackHash` (`4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`);
the requested project log and world-ready marker were never produced. The fault
predates PlantFactory's removal and persists with the mount absent, so that
deprecated plugin is not a sufficient explanation. Preserve this as a separate
startup diagnosis and do not repeat the launch until it can be symbolized or
its environment meaningfully changes.

## 2026-10-13 — Installed Beech is not yet a canopy solution

UE 5.8.3 can render the installed European Beech skeletal output through
`UInstancedSkinnedMeshComponent`. With the project's Nanite foliage feature
off, Beech A/B show bare branches; startup-only overrides for Nanite foliage,
voxels, and assemblies reveal the leaves, but they render as coarse, very dark
clusters in the isolated preview. The ordinary skeletal component looks
similar, so this is not yet an instancing win or an Island-ready tree. No
transform provider is assigned, and wind or batch performance remains
untested. Do not scatter the current outputs to fill the woodland midground.
Inspect the PVE graph/preset and foliage materials, then judge a lit candidate
in the fixed Tideglass-to-Woodland Game view and benchmark a small batch before
Island placement. Full evidence:
[instanced-skinned Megaplant probe](findings/2026-10-13-instanced-skinned-megaplant.md).

## 2026-10-12 — Woodland structure: keep the mid-distance art-directed

The second collisionless hazel-coppice mesh passed structural Blender/FBX
checks at 12,310 triangles but still read as disconnected angular leaf sprays
in its re-imported preview, so it is not an Island candidate. Together with
the earlier Game-rejected skeletal hazel thicket and the fern's unresolved
shadow review, this is evidence against generating more isolated procedural
plant meshes without a proven foliage material/silhouette. Next try a sparse,
art-directed grouping of assets already proven in Game, or the isolated PVE
Static Mesh export once the UE editor is stable; judge it in the fixed
Tideglass-to-Woodland Game view before any map placement. See the
[coppice candidate review](findings/2026-10-12-woodland-hazel-coppice.md).

## 2026-10-12 — A small everyday ecology cue

Daytime Tideglass dragonflies now occasionally pause above the nearest existing meadow bloom, then fade back to their pool-side patrol. Flower lookup is read-only; no bloom is landed on, modified, or persisted. Visits are infrequent, suppressed in heavier rain and during raven/ripple responses, and use staggered checks plus a cooldown so the three dragonflies do not become a constant synchronized spectacle. The UE 5.8.3 editor target built and `CaptiveSky2.Agent.NightEcology` passed under NullRHI with thinking disabled; the regression checks nearest-bloom selection, hover geometry, movement toward the bloom, natural return, cooldown, storm suppression, visitor interruption and unchanged HISM instance count (`Saved/Logs/Codex_DragonflyFlowerVisit_NightEcology_Verified_20261012.log`). This is source-and-automation evidence, not yet an in-game visual review; judge how often the visits read at normal play distance during a bounded daylight run.

**Bounded daylight review (2026-10-12):** the saved Island loaded in Game mode and produced five real-RHI frames from 11:00–11:57 with agent thinking/Python disabled, an isolated data root, a 120-second runtime cap and zero permitted model requests. It exited normally after 120.2 seconds with no requests. The wildlife is visibly active (three dragonflies plus a wandering stag), but this Tideglass-to-Stones viewpoint does **not** show a dragonfly visiting a bloom, so the new cue's natural visibility remains unverified. The frames do give a strong composition diagnosis: foreground broadleaf meshes dominate, a wide brown midground remains exposed, and the distant tree line repeats a thin silhouette. [Final frame](../Playtests/Codex_DragonflyFlowerGameplay_20261012/Screenshots/005_Tideglass.png); [runtime log](../Saved/Logs/Codex_DragonflyFlowerGameplay_20261012.log). The next high-value visual slice is selective mid-distance broadleaf structure at the woodland transition, not a global foliage-density increase. The four installed Megaplant tree outputs are SkeletalMeshes rather than drop-in HISM meshes; UE 5.8's `UInstancedSkinnedMeshComponent` supports per-instance transforms and animation indices via transform-provider data, making it a promising but unmeasured route. Validate one Beech/Maple instance's material compatibility, ref-pose/wind appearance and frame cost before scattering any. The no-collision, 5,778-triangle procedural hazel-thicket candidate passed structural UE import, but its isolated Island-copy Game trial showed a nearly black, skeletal crown at close and mid-distance; it is rejected for placement. The 90-second real-RHI run ended with zero model requests and left the original map unchanged ([close view](../Saved/Playtests/Codex_WoodlandHazelThicketGame_20261012/Screenshots/001_Close.png), [mid-distance](../Saved/Playtests/Codex_WoodlandHazelThicketGame_20261012/Screenshots/002_Mid_Distance.png), [finding](findings/2026-10-12-woodland-hazel-thicket.md)). The broader goal remains: add convincing mid-distance habitat structure to break up brown openings, using species and material forms already proven in Game rather than simply increasing foliage counts.

**Woodland fern Game review (2026-10-12):** a single imported fern was rendered in a disposable Island copy under real D3D12. Its green material and recognizable fan silhouette survive in Game; the initial dense placement was obscured, and moving it into a nearby opening revealed an overly sharp, black cast shadow. The 60-second run ended with zero model requests. The shadow-off setting was applied to the preview actor, but the follow-up did not reach world-ready within 300 seconds, so that fix remains unverified. Keep the fern unplaced until shadow behavior and a sparse HISM patch are validated; the larger issue is still the bare midground and the need for a convincing composition, not a blanket rise in plant count ([close view](../Playtests/Codex_WoodlandFernGameClearPatch_20261012/Screenshots/001_Clear_Patch__Close.png), [woodland-edge view](../Playtests/Codex_WoodlandFernGameClearPatch_20261012/Screenshots/002_Clear_Patch__Woodland_Edge.png), [finding](findings/2026-10-12-woodland-fern-candidate.md)).

## 2026-10-11

This week's repository state is unusually concrete: RatHollow has moved from an idea to a bounded asset/site experiment; a rat-scale PCG understory variant exists; the imported Raven and Rat pipelines are verified; Aster and the Innkeeper have MetaHuman source assets but are not yet assembled; the Island now has a runtime switch that can suppress agent thinking for zero-request PIE validation; and Tideglass Game captures show the visual bottleneck has shifted away from raw foliage count toward composition, materials, landmarks and middle-distance structure.

### 1. Make RatHollow the first **canonical microhabitat**, not merely a rat prop

**What the player sees/experiences:** Fenrus's home should read as a tiny ecological place: a broken, damp fallen-log cavity half-hidden by rat-scale understory, leaf litter, moss, disturbed soil and one or two habitual approach runs. At human height it is easy to miss. At rat height it becomes a coherent little landscape. Fenrus can leave, return, cache small objects/food, and gradually make the hollow visibly inhabited.

**Why it matters:** this is a compact proof of the north-star architecture. A home is canonical world truth (occupant, entrance, shelter quality, caches, disturbance/history), while Unreal materializes that truth with a log mesh, PCG understory, decals/material variation and navigation. It also gives wildlife agency persistent spatial consequences without pretending every animal needs an LLM civilization.

**Why it fits now:** the repo already has the disposable RatHollow trial map/site work, a clearance-tested hollow mesh, `PCG_RatHollowUnderstory_RatScale_Prototype` with 96 surveyed points in six clumps, Rat locomotion on `SK_Rat_01`, and a safe `CaptiveSky.Agent.DisableThinking` runtime gate. The current review explicitly says the pale regular cylinder on brown ground is not visually acceptable yet and needs runtime understory context.

**Smallest practical implementation:** add a renderer-independent `WildlifeHomeRecord` (stable place ID, species/occupant, entrance anchor/orientation, shelter state, cache list, last-used/disturbance timestamps) and one RatHollow materializer. Do not authorize the main-Island placement until the existing bounded PIE/Game concealment + nav review passes. The first art pass should make the hollow irregular/broken and mossy rather than adding more simulation.

**First visual milestone:** paired human-height and rat-height real-RHI frames of the same approved hollow: from human height it disappears into woodland-edge structure; from rat height the entrance, run, damp wood, litter and shelter geometry read immediately. A high-quality fallen-log/bark/moss asset set would have disproportionate impact.

### 2. Replace the remaining blockout read with a **Tideglass material-and-landmark grammar**

**What the player sees/experiences:** the existing Tideglass composition stops looking like a prototype even before any settlement is built. The pool, approach, Listening Stones/standing stones, inn edge, exposed ground and woodland transition share a believable material language: geological rock variation, weathering, lichen/moss, compacted foot traffic, wetness near water, proper wood/stone surfaces, and deliberate mid-distance masses. The eye reads a place rather than isolated systems.

**Why it matters:** this is the highest-return visible work now. Recent standalone Game captures are already around the 30-FPS floor or better in the tested Tideglass compositions, while the repo repeatedly records the same visual failure: tangled foreground, broad brown mid-distance, thin tree silhouettes, blockout-looking stones/inn edges, and some imported foliage/material fallbacks. More global foliage density is no longer the answer.

**Why it fits now:** the Island already has Tideglass water/life, wet-edge vegetation, `PN_interactiveSpruceForest`, transient lichen on Listening Stones, StarterContent instanced-material repairs, a repeatable 11:00/17:00 capture path and CSV performance harness. This is the natural next step after last week's ecological-composition recommendation: keep the semantic habitat bands, but now make the surfaces and landmark silhouettes carry their share of the image.

**Smallest practical implementation:** choose the existing fixed Tideglass-overlook Game camera and make one bounded “beauty slice”: (1) replace/upgrade the three most conspicuous blockout stone/landmark surfaces; (2) add a deterministic compacted-path/ground-breakup layer through the approach; (3) fix the remaining relevant instanced-material fallbacks used in that view; (4) add two or three authored middle-distance shrub/rock/tree masses rather than increasing global density. Re-profile the identical camera.

**First visual milestone:** the 11:00 fixed Game shot should be publishable without explaining away white/default surfaces, brown empty ground or proxy stones, while retaining a p95 above the project's 30-FPS floor. A small pack of excellent local rock faces, ground/soil blends, bark and mid-distance vegetation clumps would buy more than another million instances.

### 3. Give Aster a **persistent craft lineage**: baking as the first non-monumental culture

**What the player sees/experiences:** Aster makes a particular bread because she has learned and refined it, not because a “bakery” prop exists. Batches differ slightly; successful forms recur. The Innkeeper can taste or request one, another resident can learn it, and later the player can encounter a recognizably related loaf, preparation note, serving custom or ingredient choice elsewhere. Culture begins to propagate through ordinary excellence rather than only through stone art.

**Why it matters:** last week's cultural recommendation focused on visual motifs in stone arrangements. This is deliberately different: transmit **practice**. A culture should contain recipes, techniques, standards and remembered exemplars as well as objects. It also gives relationships and NPC-to-NPC interaction concrete consequences: teaching changes what another person can make.

**Why it fits now:** Aster and the Innkeeper are already persistent residents; the Inn/guest-book and resident memory/world-state machinery exist; Aster's MetaHuman source asset has now been prepared even though final assembly is pending. The canonical layer can hold the craft evidence without waiting for hand animation or a finished kitchen.

**Implementation status (2026-10-12):** the current Unreal world-state subsystem now has a bounded, persistent craft-tradition ledger: attempts record practitioner, source, day, up to four normalized parameters, and an outcome; lessons record attributed teacher/learner links. A resident may optionally practice a known tradition at the lit, nearby inn hearth, and may optionally teach one they know during an in-range resident conversation. The recipient gains knowledge only after that speech is delivered. The system does not seed a recipe, spawn ingredients or a loaf, or make an extra model request. This is renderer-independent world data inside Unreal, not yet the engine-agnostic canonical service proposed above. See [craft-lineage validation](findings/2026-10-12-craft-lineage-validation.md).

**Remaining implementation:** let Aster choose the first practice through her normal scheduled decision, import and visually review the separate bread-prop revision, and later connect practice outcomes to a visible batch only when ingredients, evaluation, and object persistence are defined. Do not write an Aster recipe into memory or present an unrated experiment as a successful loaf. Teaching remains optional and uses existing scheduled decisions; no background model calls are added.

**First visual milestone:** a small inn table at warm evening light showing three related but non-identical loaves/batches, a practical preparation surface, and Aster/Innkeeper in a teaching or tasting moment; later a derivative loaf appearing elsewhere should be legible evidence that knowledge travelled. One excellent bread/crumb/flour/board prop set plus a restrained hand/serve animation would carry this much further than a large kitchen asset pack.

## Concept art — Tideglass becomes materially believable before it becomes a city

**Direction / reproducible prompt:** Wide cinematic 16:9 view from a shaded working terrace above Tideglass at late-afternoon golden hour. Keep the settlement modest: the existing inn and only a handful of useful structures, terraces and paths, not a finished metropolis. Foreground: Aster works at a practical table with bread, flour, a board and simple tools while another resident observes/tastes; nearby are sketchbook or crafted objects showing ordinary creative life. Middle ground: a compacted path descends through flowering meadow and woodland-edge masses toward clear Tideglass water; local stone retaining edges and steps use the same geology as nearby outcrops, with moss/lichen and damp darkening where appropriate. Include a subtle rat-scale hollow under roots/log at the woodland edge—nearly invisible at human scale—and a raven overhead. Listening Stones should look geological and weathered, not like pale proxies. Architecture is technology-agnostic and fit for purpose: precise contemporary glazing or metal connections may coexist with stone, timber and repaired older fabric where each makes functional sense. No medieval costume, fantasy ornament, cyberpunk, or imposed futurism. Show evidence of accumulated use: worn path centers, planted pockets, repaired surfaces, small personal objects, garden work, and one resident-made artwork. Water reflections, atmospheric mountain/coastal depth, layered vegetation with deliberate negative space, warm interior light beginning at the inn, long soft shadows, gentle wind. Materials are the star: wet stone, mineral soil, bark, lichen, glass, metal, timber, cloth, bread and foliage all distinct. Mood humane, calm, intelligent and alive. The scene should feel beautiful because ecology, craft and repeated human choices have cohered over time—not because a genre aesthetic was imposed.

**Generated concept image:** generated in the weekly run and delivered in ChatGPT. Binary image persistence is not available through the connected GitHub text-file writer, so no repository image path is claimed.

## 2026-10-04

### 1. The first canonical resident project: a crossing that becomes a path

Use a small resident-built crossing as the first proof that canonical world projects can evolve independently of Unreal and later materialize correctly.

A resident identifies an inconvenient crossing, forms an intention to improve it, selects a validated site, gathers materials, and gradually constructs something. The observer can leave for several Island days and return to find a material cache, cleared vegetation, supports, a partial deck, and eventually a usable crossing. Once complete, NPC routing changes and repeated traffic begins producing a worn approach.

Implement a renderer-independent project record with a stable project/site ID, proposer, purpose, progress, resources, and bounded world-modification operations. Unreal materializes those facts rather than owning them. Start narrowly: one CrossingProject, one approved location, and four or five deterministic construction states. An LLM may originate intent, but deterministic systems validate the site and operations.

**Current architecture fit (source audit, 2026-10-05):** `UIslandWorldStateSubsystem` already owns per-map lasting records outside `Saved/`, parses and writes a versioned JSON document, protects unreadable state from overwrite, and atomically replaces the file after a successful save. Nests, curios, stone-arrangement sites, and guest-book entries are serialized there; nest and arrangement actors are materialized from those records. This is the right canonical owner for a crossing project, with an explicit round-trip/migration test before any stage can persist. Today, the generic resident `build` dispatcher accepts arrangement sites and the guest book; raven foraging and nest weaving are specialized in `RavenAgentAIController`. No shared staged-project record or progress operation exists yet.

**Current save evidence (2026-10-05):** `WorldState/Island.json` contains the shared cairn at four stones, last changed on Island day 3, with Aster recorded as a contributor; the saved clock is on day 4. This confirms the existing bounded resident-made change is in persistent world state. It does not demonstrate multi-stage construction or autonomous project progress. A tracked-source/config search found no `CrossingSite` marker; the binary Island map still needs an in-editor terrain, water, and navmesh inspection before selecting a fixed approved site or exposing a resident build target.

**Candidate-site check (2026-10-05):** the current editor actor inventory reports a `WaterZone` and `WaterBodyOcean`, but no stream/river body or authored crossing. Tideglass itself is a static-mesh footprint with cardinal shore stones, not a separate water body. The existing [overhead capture](../Saved/Playtests/Codex_TideglassOverhead_Elevated_20261001/Screenshots/001_Overhead.png) shows a compact pool with apparently open ground around it, while the [water notes](ecology/tideglass-water.md) establish that its generated surface is visual-only and non-colliding; the original footprint retains collision. This is not enough to declare the pool a bad site, but it is also not evidence that it is an inconvenient crossing.

The new read-only `CaptiveSky2.Agent.CrossingSiteAudit` ran against the loaded saved Island in UE 5.8.3. Opposite banks on both pool axes projected to navmesh; each baseline route was exactly the 8.2 m straight chord (1.00×). A visibility trace across X hit the pool footprint, but navigation still reported a direct route; the Y trace hit a different static mesh. Thus Tideglass does not currently create a navigation detour for these bank pairs and cannot support the proposed route-shortening payoff. Reject this as the first crossing site; keep the audit for rechecking if the pool/navmesh changes. Log: [`Codex_CrossingSiteAudit_20261005.log`](../Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_CrossingSiteAudit_20261005.log).

For a different site, capture the same two grounded endpoints before and after construction: first, record a complete baseline nav route and its length; then prove the proposed span crosses a real obstruction and materially shortens that route; finally, verify a completed span yields a complete route and physical traversal, with the project reset restoring the original route. Site selection remains open; inspect a different map location rather than manufacturing a crossing over an already convenient path.

The editor-only `CaptiveSky2.Agent.CrossingSiteAudit` keeps the rejected Tideglass measurement and screens opposite-bank corridors around the other authored landmarks at four compass orientations and five bounded distances. A candidate must project to walkable navmesh on both sides, have a complete current route at least 1.2× its straight chord and 5 m longer, and have a blocked visibility trace against a static mesh that affects navigation and blocks pedestrians. Landmark/building/wildlife blockers and traces within 5 m of the landmark are excluded. This is a shortlist heuristic, not an approved site: inspect obstacle and terrain in the saved map, verify water/footprint clearance, and later prove a completed span shortens the route and supports physical traversal.

**Current saved-Island audit (2026-10-05):** the UE 5.8.3 editor automation ran against the saved Island with nav data ready. Tideglass again measures an 8.2 m straight chord and 8.2 m nav route on both tested axes; its X visibility trace hits the pool footprint, but neither axis has a route detour. The added landmark-centered screen found no corridor meeting all four conservative conditions (complete nav route, at least 1.2× detour, at least 5 m bypass, blocked direct trace). This is not evidence that no suitable crossing exists elsewhere. It means there is no validated candidate yet: inspect other terrain corridors in the map before naming a fixed site, and do not add a resident build target or construction authorization until water, terrain, routing and traversability are checked. Log: [`Codex_CrossingSiteAudit_Current_20261005.log`](../Saved/Logs/Codex_CrossingSiteAudit_Current_20261005.log).

**Expanded saved-Island audit (2026-10-06):** the UE 5.8.3 editor target built and the read-only automation passed with nav data ready. An initial expanded sweep surfaced four apparent detours, but all were false positives: three crossed the Inn's own wall or plinth, and the fourth hit `ListeningStone_C`. The audit now rejects tagged landmark/Inn/wildlife blockers, any blocker that is not both navigation-relevant and pedestrian-blocking, and traces within 5 m of a landmark center. The saved nav bounds are approximately 108.7 × 108.7 m; a second pass sampled a 15 m grid across the walkable bounds, four bearings, and five spans from 15 to 60 m. No corridor passed the combined screen. Log: [`Codex_CrossingSiteAudit_FullGrid_20261006_02.log`](../Saved/Logs/Codex_CrossingSiteAudit_FullGrid_20261006_02.log). This is stronger negative evidence, not an exhaustive geometric or visual survey; no crossing should be authorized until a promising corridor is visually inspected and a real traversal benefit is measured.

**Current saved-Island recheck (2026-10-08):** a headless UE 5.8.3 editor startup loaded `/Game/Maps/Island` with nav data ready, and the read-only `CaptiveSky2.Agent.CrossingSiteAudit` passed. Tideglass still has complete 8.2 m straight routes on both sampled axes (1.00× chord); the X-axis visibility trace hits the pool footprint but does not create a nav detour. The bounded landmark and 15 m grid probes again found no corridor that combines a complete route, at least 1.2× detour, at least 5 m bypass, and a pedestrian-blocking navigation obstacle outside protected landmark space. Log: [`Codex_CrossingSiteAudit_20261008.log`](../Saved/Logs/Codex_CrossingSiteAudit_20261008.log). The saved-map/nav evidence is current, but it is not a visual survey of every terrain fold; keep the bridge idea open and do not name a site or authorize construction until an actual route-shortening, traversable location is found.

**Denser saved-Island recheck (2026-10-09):** UE 5.8.3 built the updated audit in an isolated scratch project and the headless `CaptiveSky2.Agent.CrossingSiteAudit` passed with nav data ready, zero model requests, and `NullRHI`. The global grid spacing is now 7.5 m (four times as many grid centers as the 15 m pass), with the same four bearings, five radii, and conservative detour/obstruction checks. Tideglass remains two straight 8.2 m routes. The denser pass produced one lead: center X=-99,204 Y=101,598, NW-SE banks 30.0 m apart, current route 52.3 m (1.74×; 22.3 m bypass). The audit identifies the direct-trace blocker as an untagged `StaticMeshActor` using `/Game/PN_interactiveSpruceForest/Meshes/half/high/spruce_half_01`; it is spruce geometry, not evidence of a stream, ravine, unsafe bank, or traversable span. Treat the lead as a vegetation-obstacle false positive and reject it as a crossing site. No site is approved; a different location still needs visual inspection and a measured route-shortening/traversal benefit before any project marker or resident build action. Log: [`Codex_CrossingSiteAudit_Dense_20261009.log`](../Saved/Logs/Codex_CrossingSiteAudit_Dense_20261009.log).

**Vegetation-filtered crossing recheck (2026-10-09):** the read-only audit now rejects an otherwise qualifying corridor when its direct-trace actor/mesh name looks botanical, and reports the matched asset paths. The isolated UE 5.8.3 build succeeded and `CaptiveSky2.Agent.CrossingSiteAudit` passed with `NullRHI`, thinking disabled, zero model requests, and a 60-second realtime cap. It filtered exactly one probe, `/Game/PN_interactiveSpruceForest/Meshes/half/high/spruce_half_01`, then found no other corridor in its bounded grid/radius sweep. This strengthens the rejection of that spruce false positive; the name/path classifier may miss unfamiliar vegetation naming or reject a non-vegetation asset with a botanical name, so this is not an exhaustive map survey or proof that no suitable crossing exists. No map, world state, or authored Content was changed. Log: [`Codex_CrossingSiteAudit_NoVegetation_20261009.log`](../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_CrossingSiteAudit_NoVegetation_20261009.log).

The crossing therefore needs a deterministic project operation and a validated fixed site, not a one-off visual actor or an LLM-authored transform. Progress should be bounded and derived from existing decisions/state (no added model calls or unbounded background simulation). The materializer should expose its stages—cleared verge and cache, then supports and partial deck, then a traversable finished crossing—and only the completed stage should change resident routing. Keep the JSON reset/reversal path and prove old world-state fields survive the new project field. The authored look must remain a consequence of the canonical record, not the source of truth.

**First visual milestone:** three identical camera shots at 0%, 40%, and 100%. Initially there is vegetation and an awkward crossing. At 40% there is a cleared verge, material pile, supports and partial deck. At 100% there is a modest finished crossing, changed NPC route and visibly trodden approaches.

This is also a useful temporal-resolution experiment, although the world currently may not be run frequently enough for long offline project progression to be the highest immediate priority.

### 2. Replace brute-force foliage density with ecological compositions

This week's primary visual recommendation.

Recent foliage work demonstrates that instance count alone does not create a convincing landscape. Very dense foreground cover can coexist with weak middle-distance composition and thin distant silhouettes, while imposing a substantial rendering cost.

Move from “cover the landscape” toward semantic ecological zones that compose readable habitats. Initial examples:

- TideglassWetEdge — reeds and moisture-loving plants;
- InnMaintainedVerge — shorter cover and compacted circulation;
- WindArchExposed — windswept grasses and lower forms;
- ListeningStonesMeadow — flowering meadow pockets;
- WoodlandEdge — shrubs and understory transitioning into spruce mass.

Build these using the project's existing foliage collection, interactive spruce forest, landmark rings, exclusion logic, wind response, landscape/water awareness and visual capture/performance harness.

This also creates the right abstraction for future resident world modification. A new path should modify the canonical ecological/spatial state of a corridor; the ecological materializer should respond, rather than construction code directly deleting thousands of anonymous foliage instances.

**Smallest practical implementation:** choose one Tideglass-to-Wind-Arch camera and explicitly art-direct ecological bands for that composition. Tune density, species grouping, negative space, silhouette and LOD/performance around the view before generalizing the rules.

**First visual milestone:** an 11:00 Tideglass view that clearly reads wet edge → open circulation → meadow masses → woodland edge → mountain, while meeting the project's broad-view performance target.

A small number of excellent grass/flower clumps, rock assets and convincing distant-tree silhouettes may have disproportionate visual impact.

**Highlight-capture audit (2026-10-06):** the bounded late-afternoon captures make the composition problem tangible. The [shore approach](../Saved/Playtests/Codex_HighlightFast_20261006_2220/Screenshots/001_Shore_Approach.png) reads as a broad brown plane with widely separated tree silhouettes and little mid-distance structure. The [Tideglass view](../Saved/Playtests/Codex_HighlightFast_20261006_2220/Screenshots/005_Tideglass.png) has a much denser local plant ring, but foliage obscures much of the pool edge and nearby small-scale activity. The [Wind Arch overlook](../Saved/Playtests/Codex_HighlightFast_20261006_2220/Screenshots/008_Wind_Arch_Overlook.png), taken at 17:34, is strongly backlit with near-black foreground forms; treat this as a lighting/composition issue, not evidence that the plants themselves are dark. For the next vegetation pass, judge one repeatable 11:00 Tideglass-to-Wind-Arch view: preserve a legible circulation opening, shape a distinct wet-edge band rather than a uniform dense ring, add readable meadow/woodland mass in the middle distance, and compare brightness and frame time against a recorded baseline. These frames are visual evidence only; no PIE performance or interactive walkability was measured here.

**Fixed-view Game baseline (2026-10-07):** a standalone 1600×900 Game spectator session held its first TideglassPool view stationary and captured 600 frames after a 15-second warm-up; the session ended at its 90.2-second play cap with zero model requests. FrameTime was p50 16.09 ms, p95 37.45 ms (~26.7 FPS), mean 21.59 ms (~46.3 FPS), and max 438.65 ms. The stationary broad view therefore still misses the 30-FPS p95 target, with a long hitch tail whose cause is not identified by this summary. The runtime log reports only 10 nonblocking cattails in the Tideglass wet-edge scatter, placed outside the 306 cm pool-edge clearance; this is not yet the clearly shaped, readable wet-edge band in the target composition. It also warns that 13 `/Game/Plants/Materials/OriginalFlower/MI_Rhodo_Ever_*` material instances lack `InstancedStaticMeshes` usage and fall back to Default Material in Game. Audit and repair those imported material assets before judging their appearance or expanding their use; this pass did not modify binary Content. An earlier 600-frame sample from the default 24-second shot cycle had p95 46.45 ms and crossed a shot-cycle boundary; all five entries in its viewpoint file are identical, so no camera relocation occurred. Keep that sample separate rather than attributing its worse tail to camera motion. This is standalone Game evidence, not PIE/player traversal. The profile is [`Profile(20261007_141629).csv`](../Saved/Profiling/CSV/Profile%2820261007_141629%29.csv) and its session log is [`Codex_VegetationFixedView_20261007.log`](../Saved/Logs/Codex_VegetationFixedView_20261007.log). The repeatable [11:00 editor-world frame](../Saved/Viewpoints/2026-10-07_135710_h11.0/02_Tideglass.png) shows lush foreground cover but still a broad sparse brown midground and blockout landmark proxies; it is composition evidence, not the Game frame itself. Next: art-direct a Tideglass-to-Wind-Arch habitat sequence—readable wet-edge, circulation opening, meadow mass, and woodland silhouette—then remeasure the same fixed Game view and a separate PIE traversal before attributing costs or calling the 30-FPS target met.

**Tideglass wet-edge preview (2026-10-07):** the bounded `CaptiveSky2.Visual.Viewpoints` editor-world capture passed with `-Hour 11 -Only Tideglass -GroundCover -NoWorldState`. It made no play session or resident/model turn and did not load/save world state. The transient preview placed 1,780,640 nonblocking ground-cover instances within the 1,920,000 candidate budget; its wet edge contained 7 Typha and 3 Phalaris, with 559.6 cm minimum pool-edge clearance and 440.8 cm minimum shared spacing. SceneCapture throughput was 39.82 FPS wall-clock and 39.54 FPS p95 at Tideglass, and 58.93 / 54.00 FPS in the close ground-detail view. Both clear the harness's 30-FPS p95 screening threshold, but these are offscreen editor-world captures—not Game, PIE, or target-hardware guarantees—and should not be treated as evidence that normal player traversal is fixed. The [Tideglass overview](../Saved/Viewpoints/2026-10-07_145831_h11.0/02_Tideglass.png) still shows an overgrown foreground, a sparse brown midground, and white/black-and-white blockout-looking water/landmark surfaces; the [ground-detail crop](../Saved/Viewpoints/2026-10-07_145831_h11.0/02a_TideglassGroundDetail.png) is almost entirely foliage, so the wet edge is not yet compositionally legible. Log: [`Codex_TideglassGroundCover_20261007.log`](../Saved/Logs/Codex_TideglassGroundCover_20261007.log). Next: use the same camera to art-direct a narrower, visibly separated wet-edge band and open circulation lane, then verify material presentation in standalone Game and remeasure PIE separately.

**Matched foliage-layer isolation (2026-10-07):** the existing diagnostic rendered `meadow` and `woodland` separately at the same 11:00 Tideglass camera, with no world-state writes. Both `CaptiveSky2.Visual.Viewpoints` runs passed. The [meadow-only frame](../Saved/Viewpoints/2026-10-07_160514_h11.0/02_Tideglass.png) keeps 19 HISM components visible and reaches 33.41 FPS p95; it is lush in the near ground but leaves the middle distance largely brown. The [woodland-only frame](../Saved/Viewpoints/2026-10-07_160651_h11.0/02_Tideglass.png) keeps three components, reaches 47.53 FPS p95, and restores a readable tree line but does not fill the bare midground. These are editor-world SceneCapture rates, not Game/PIE performance. Both frames also use the editor fallback white pool and proxy landmarks, so judge only the vegetation layers. The evidence points to a spatial-composition problem, not a need to raise global density: the next pass should thin foreground cover, introduce a few legible mid-distance meadow/understory masses, and preserve a clear approach to the pool.

**Repeat meadow-layer diagnostic (2026-10-07):** the updated `CaptiveSky2.Visual.Viewpoints` path passed a no-world-state Tideglass capture with `-GroundCoverSoloLayer=meadow`. It kept 19 HISM components visible; p95 capture throughput was 35.73 FPS at the overview and 39.01 FPS at ground detail. The transient wet edge counted 7 Typha plus 3 Phalaris, with 559.6 cm minimum pool-edge clearance and 440.8 cm shared spacing. This is one offscreen editor-world sample, not a matched performance improvement over the earlier layer capture and not Game/PIE evidence. The [new frame](../Saved/Viewpoints/2026-10-07_180635_h11.0/02_Tideglass.png) still shows dense foreground cover against a broad sparse midground; white water and proxy landmarks are editor-world placeholders, not a judgment of the current Game material path. Log: [`Codex_TideglassMeadowLayer_20261007.log`](../Saved/Logs/Codex_TideglassMeadowLayer_20261007.log). Next, evaluate the full habitat composition in a matched standalone Game view and a separate PIE traversal before concluding either art or the 30-FPS target is solved.

**Flower-pocket population recheck (2026-10-10):** the layer-isolation capture work added an explicit cap: 251–512 Fab meadow flowers, and 1,501–2,100 combined meadow flowers and woodland rhododendrons. The previous 512 candidate sites produced 1,003 meadow flowers plus 1,487 rhododendrons (2,490 total), failing that contract. Reducing the deterministic candidate pool to 256 retained the 160-site Listening Stones reservation; the saved-Island editor preview placed 295 meadow flowers (21 in the Listening Stones meadow) and 1,487 rhododendrons, 1,782 total. `CaptiveSky2.Visual.Viewpoints` passed at 11:00 with the meadow-understory layer isolated, the three optional dragonflies staged, and `-NoWorldState`. SceneCapture p95 was 42.07 FPS at the overview, 42.88 at ground detail, and 51.68 in the dragonfly close-up; all exceed the diagnostic 30-FPS floor, but this editor-only layer sample is not Game/PIE performance. The wide image still shows dense foreground against a sparse brown middle distance and editor proxy water/landmarks, so it informs ecological density/composition, not final art. Evidence: [capture log](../Saved/Logs/Codex_Tideglass_MeadowUnderstory_20261010_Final.log), [overview](../Saved/Viewpoints/2026-10-10_094139_h11.0/02_Tideglass.png), [close detail](../Saved/Viewpoints/2026-10-10_094139_h11.0/02a_TideglassGroundDetail.png), and [dragonfly view](../Saved/Viewpoints/2026-10-10_094139_h11.0/02e_TideglassDragonflyClose.png). Details: [flower-pocket population finding](findings/2026-10-10-meadow-flower-cap.md).

**Mid-distance scatter experiments (2026-10-10):** two reversible UE 5.8.3 code probes changed only where the existing fixed meadow budget is spent: the general-fill landmark exclusion radius was narrowed from 55 m to 35 m, then landmark patches were raised from 12 to 18 per anchor with the original clearance restored. `CaptiveSky2.Visual.Viewpoints` passed for the no-world-state captures. The radius change admitted 17 additional valid terrain cells and only 623 more broad-cover instances (1,780,040 → 1,780,663); the patch change left the overall preview essentially flat (1,780,640 → 1,780,745). The matched images still read as lush foreground and a mostly brown far midground, without a decisive improvement, so neither code adjustment is retained. Latest combined overview SceneCapture p95 was 38.74 FPS and the 5-second interactive warm check reached 38.65 FPS; these are editor diagnostics, not Game/PIE or traversal proof. Logs: [closer landmark clearance](../Saved/Logs/Codex_Tideglass_MeadowUnderstory_CloserEdge_20261010.log), [combined clearance probe](../Saved/Logs/Codex_Tideglass_CombinedCover_CloserEdge_20261010.log), [anchor patch probe](../Saved/Logs/Codex_Tideglass_AnchorMeadowPatches_20261010.log); [latest frame](../Saved/Viewpoints/2026-10-10_100358_h11.0/02_Tideglass.png). Next, test actual species/material variety in spatially art-directed mid-distance groups rather than further tuning these abstract candidate counts; the Fab/Megaplants library assets are not yet present in the project's `Content` tree.

**Fixed-view Game recheck (2026-10-07):** a bounded standalone Game run used the same five identical Tideglass viewpoints, the same 1280×720 internal render resolution, and a full 600-frame CSV sample. Frame time was mean 15.71 ms, p50 14.92 ms, p95 21.29 ms (47.0 FPS), and max 51.48 ms. The earlier fixed-view sample at the same camera/resolution recorded p95 37.45 ms (26.7 FPS), so this repeat passes the 30-FPS p95 screen for this stationary view; the difference is encouraging but its cause is unproven and it does not demonstrate traversal or whole-island performance. The shot at [11:14 in Game](../Saved/Playtests/Codex_TideglassCurrentBounded120_20261007/Screenshots/002_Tideglass.png) shows the actual water and landmark presentation rather than editor proxies, while the broad midground still reads sparse against the dense near ground. Background thinking and Python were disabled; the isolated session ended normally after 120.2 seconds with zero model requests. The [CSV](../Saved/Profiling/CSV/Profile%2820261007_182316%29.csv) and [session log](../Saved/Logs/Codex_TideglassCurrentBounded120_20261007.log) are the evidence. A first 60-second attempt stopped before its requested 600-frame sample completed and is excluded. Next, keep the observed Game pass separate from a PIE traversal sample, and solve the mid-distance habitat composition without raising global density by default.

**Fixed-camera Game activity recheck (2026-10-07):** a later 1600×900 standalone Game capture held one TideglassPool establishing view for 75.3 real seconds (Island time 11:00–11:29), with thinking and Python disabled, an isolated world-data root, and zero model requests. Across the [first](../Saved/Playtests/Codex_TideglassCompositionCurrent_20261008/Screenshots/001_Composition__Current.png) and [last](../Saved/Playtests/Codex_TideglassCompositionCurrent_20261008/Screenshots/003_Composition__Current.png) frames, the stag moves across the clearing and the three dragonflies occupy different positions, giving a small visual confirmation of ambient wildlife motion without an unbounded play run. This was a visual sample, not an FPS profile or PIE traversal. The frame still has a dense, partly overgrown foreground, broad sparse brown midground, a nearly circular pool surface, and blocky landmark silhouettes; it is not yet the intended wet edge → circulation → meadow → woodland composition. The run again logs 13 distinct Rhodo Ever material instances without `InstancedStaticMeshes` usage, so those imported materials fall back to Default Material in Game; repair that asset issue before using the affected species to judge final color or expanding their coverage. Log: [`Codex_TideglassCompositionCurrent_20261008.log`](../Saved/Logs/Codex_TideglassCompositionCurrent_20261008.log). Next: fix the imported-material usage in the authored assets with explicit Content coordination, then repeat the same view and profile PIE separately.

**Repeat fixed-view Game performance sample (2026-10-07):** the same TideglassMotionProbe camera and 1600×900 spectator run captured all 600 requested frames after a 15-second delay. FrameTime was mean 16.89 ms, p50 14.92 ms, p95 24.15 ms (41.4 FPS), and max 173.44 ms. The p95 clears the 30-FPS screen for this stationary composition, but the maximum shows that this is not a per-frame guarantee. In the worst frame, RenderThreadTime was 139.11 ms, GPUTime 24.44 ms, and the largest exclusive RenderThread samples were `EventWait` 52.91 ms, `RDG` 35.53 ms, and `EventWait/Visibility` 15.15 ms. This is not evidence of a 139 ms CPU render pass; synchronization/resource work contributes materially, but the cause remains unproven and needs a trace or matched PIE capture. This remains standalone Game, not PIE traversal or a whole-island claim. Thinking and Python were disabled, the world-data root was isolated, the run ended at its 120.1-second cap with zero model requests, and no source or Content asset was modified. The [CSV](../Saved/Profiling/CSV/Profile%2820261007_231924%29.csv), [screenshot](../Saved/Playtests/Codex_TideglassGameProfileCurrent_20261007/Screenshots/001_Tideglass.png), and [log](../Saved/Logs/Codex_TideglassGameProfileCurrent_20261007.log) are the evidence. That log again reports 13 Rhodo Ever instances plus Starter Content `M_Rock` and `M_Wood_Oak` without `InstancedStaticMeshes` usage, so 15 distinct material assets fall back to Default Material in Game. Next: coordinate and repair the affected imported materials, then sample matched gameplay-scale PIE traversal and investigate the frame-time tail separately.

**Transient water/landmark art probe (2026-10-07):** a no-world-state capture applied raw `/Game/Materials/M_TideglassPool_Lively` to a temporary irregular pool surface and rendered temporary `SM_Rock`/`M_Rock` silhouettes over the six Wind Arch/Listening Stones proxies; automation confirmed the original proxy meshes, transforms and collision stayed intact and the temporary visuals were removed after capture. This was not a faithful preview of the current Game water path: Game loads `/Game/Materials/M_TideglassPool_GrazingReadable` and applies the reversible `TuneReadablePoolMaterial` runtime parameters. The [transient frame](../Saved/Viewpoints/2026-10-07_150333_h11.0/02_Tideglass.png)'s flat cyan patch therefore does not reject the current runtime candidate; the matched Game capture and iteration notes in [`tideglass-water.md`](ecology/tideglass-water.md) are the relevant evidence for that material. The raw `SM_Rock` fit does expose a separate limitation: scaling one mesh to proxy bounds creates thin, dark shard-like silhouettes rather than convincing pillars. The combined ground-cover screen failed its 30-FPS p95 gate at 28.67 FPS (38.45 FPS wall-clock); the close ground-detail crop remained at 53.74 FPS p95. The first capture passed at 39.54 FPS p95, so this single preview does not establish that either override caused the regression; repeat a matched, warmed run before attributing cost. Log: [`Codex_TideglassArtPreview_20261007.log`](../Saved/Logs/Codex_TideglassArtPreview_20261007.log). Next art pass should address landmark-specific silhouettes independently and keep evaluating the current water candidate in matched Game captures.

**Quiet resident presence (2026-10-07):** the local stag routine now has a small response between explicit inspection (which startles it) and ignoring a nearby being: while otherwise grazing, it can play its existing look-around animation when a conscious resident is visible within 6 m and moving no faster than 1.8 m/s. The one-shot response does not move the animal, speak, write memory/world state, or request a model; it rearms after the nearby group leaves an 8.5 m radius, with an additional short cooldown. The UE 5.8.3 editor target built successfully. Headless `CaptiveSky2.Agent.WoodlandDeer` passed with `-NullRHI -NoSound -DisablePython`, agent thinking disabled, a zero model-request cap, and an isolated data root: [`Codex_StagNotice_20261007_Release.log`](../Saved/Logs/Codex_StagNotice_20261007_Release.log). The test covers the one-shot response, unchanged position/startle state, no model request, duration, cooldown, and re-arm after leaving. A first sandboxed launch could not access Unreal's per-user cache and stalled in Turnkey; the elevated retry completed. This verifies deterministic behavior, not the animation's in-world readability or ordinary play pacing. Next: judge the response in a bounded Game/PIE session once the interactive editor is available.

**Matched Game profile recheck (2026-10-08):** the bounded Tideglass Game sample used the existing 1600×900 fixed camera, a 600-frame CSV profile, isolated data root, disabled thinking/Python, and a 120-second realtime cap; it ended with zero model requests. FrameTime was 17.17 ms mean, 14.80 ms p50, 24.83 ms p95 (~40 FPS), and 171.78 ms max. This clears the stationary-view p95 floor but still shows a hitch tail; it is not a moving-play or PIE guarantee. The frame shows a reflective pool, tangled foreground, sparse brown middle distance, and thin tree silhouettes. Keep the wet-edge/open-lane/meadow/woodland composition work focused on that same camera, and recheck p95 after each material change. Evidence and run limits: [`2026-10-08-tideglass-performance-and-composition.md`](findings/2026-10-08-tideglass-performance-and-composition.md).

### 3. Turn persistent stone arrangements into the first artistic tradition

The existing persistent stone arrangements already provide the first half of a cultural system: residents/visitors can make forms, the works persist and weather, and residents can encounter and remember them. The next step is transmission.

Allow a resident to perceive the form/motif of an arrangement, remember it, value or discuss it, and later create something recognizably influenced by it without simply copying it. Another resident can then respond to that work.

Avoid a single CultureScore. Store small renderer-independent cultural-evidence records: observed work, perceived motif, time, attribution when genuinely known, and optionally later works that cite experienced influences.

The causal chain should remain reconstructable:

make → perceive → value → remember → imitate → transform → teach → preserve

**Smallest practical implementation:** add one motif descriptor/influence record to existing arrangements and allow one later arrangement to choose a previously perceived motif as an influence.

**Implementation progress (2026-10-06):** the first bounded transmission step is represented in the canonical arrangement records. An unobstructed first sighting records the observer (up to 32 residents per work) and a private memory containing only visible form and site; title and intent remain private. A later arrangement may name an exact known source, but the world-state validator rejects unknown sources and same-form copies. Descendants store their source site ID, and later observers can read that public lineage without learning the maker's private meaning. Residents can optionally teach the same public form/site through an existing nearby resident conversation; the recipient records a distinct attributed lesson, never a false sighting, and may transform that taught source into another form. Lessons are capped at 32 per work and persist with the arrangement. This adds no model call or conversation turn and cannot be used for player, Discord, or external messages. The UE 5.8.3 editor target built successfully; headless `CaptiveSky2.Agent.IslandArrangement` and `IslandArrangementInspection` both passed, covering teaching persistence, eligibility, learner context, and transformation: [`Codex_ArrangementTeaching_20261006.log`](../Saved/Logs/Codex_ArrangementTeaching_20261006.log). Earlier visual-marker validation is documented in [`Codex_IslandArrangement_Lineage_20261006.log`](../Saved/Logs/Codex_IslandArrangement_Lineage_20261006.log). Deterministic tests prove bounded state and wording—not that residents will choose to teach or carry motifs forward in real play, nor that the actual Island presentation reads beautifully. Resident valuation and a richer visual motif language remain future work.

**Visual continuity progress (2026-10-06):** every arrangement now has a stable seed for a small pale three-stone mark; a transformed descendant inherits its cited work's motif seed while keeping a different main form. The automation checks identical local marker transforms before and after world-state reload. This makes lineage visible in the artifact rather than only in the description, but normal-distance readability and whether residents notice or reuse motifs during play have not been verified. Optional private valuation guidance is documented below; resident choice to form such a memory remains unverified.

**Explicit-inspection continuity progress (2026-10-06):** a close, unobstructed resident inspection now records the resident's first encounter with a completed arrangement in its existing bounded `ObservedBy` evidence, allowing a later different-form work to cite that source without relying on an earlier background-perception tick. Inspecting a descendant reports its public three-stone lineage (source form and site) while continuing to withhold both makers' private titles and intent. Arrangement visibility aims just above its ground-level actor origin to avoid tracing only to the floor. The UE 5.8.3 editor target built successfully; headless `CaptiveSky2.Agent.IslandArrangement` and `IslandArrangementInspection` both passed, including the sequence inspect spiral → make line citing it → inspect the descendant. Log: [`Codex_ArrangementInspectAsInfluence_20261006.log`](../Saved/Logs/Codex_ArrangementInspectAsInfluence_20261006.log). This verifies the deterministic link, not whether residents will voluntarily inspect or transform motifs during long play, nor its visual readability at normal distance. No extra model action or request was added. Resident valuation was still future work at this point; its optional private-reflection follow-up is documented below.

**Optional private valuation (2026-10-07):** the existing decision prompt now explicitly allows a resident to use its normal optional `new_memories` field to keep a short personal impression of a completed stone form it actually saw or was taught. The note is private to that agent, does not infer the maker's intent, and is neither required nor converted into a shared cultural score. This adds no action, model call, or world-state record. A prompt contract test and the existing arrangement persistence/lineage suites verify those boundaries; see [`arrangement-reflection.md`](findings/2026-10-07-arrangement-reflection.md). The instruction makes reflection available, not likely: a bounded live-model/play run would still be needed to learn whether Aster or the raven elects to form and retain such a memory.

**Visible motif vocabulary (2026-10-07):** a persistent work's stable motif seed now selects one of three small three-stone glyphs—a path, fork, or triangle—and places it in the clear band between the main work and later responses. Descendants inherit the same motif seed, so the family resemblance survives a change in the main arrangement form; no new world-state field or model request was added. The UE 5.8.3 editor target built successfully, and `CaptiveSky2.Agent.IslandArrangement` plus `IslandArrangementInspection` passed, including deterministic glyph layouts and source/descendant transform equality. Log: [`ArrangementMotifs.log`](../Saved/Playtests/Codex_ArrangementMotifs_20261007/ArrangementMotifs.log). The noon frame at [`03_ListeningStones.png`](../Saved/Viewpoints/2026-10-07_055721_h12.0/03_ListeningStones.png) is an editor-world view of unchanged map proxies—not the Game/PIE landmark presentation—so its blue blockout pillars do not describe the runtime art. Game/PIE replaces those proxies with transient collisionless Starter Content rock forms; `CaptiveSky2.Agent.ListeningStonePresentation` covers replacement and restoration. A focused editor preview with transient M_Rock visuals and meadow understorey measured 52.34 FPS mean / 31.85 FPS p95. It exposed M_Rock's harsh black-white mottling, so the runtime forms now use the same known Color-parameter engine stone surface as other island stones, tinted to a subdued warm gray. The UE 5.8.3 build and `CaptiveSky2.Agent.ListeningStonePresentation` regression both passed. A matched noon Game capture with thinking disabled, isolated world data, and a 45-second cap ended after 45.2 seconds with zero model requests: [`001_Stone_Presentation_Close.png`](../Saved/Playtests/Codex_StonePresentation_20261007/GameScreenshotsTintFinal/001_Stone_Presentation_Close.png); log: [`StoneTintGame_Final.log`](../Saved/Playtests/Codex_StonePresentation_20261007/StoneTintGame_Final.log). On a directly lit rock face, the tint reduced a sampled pixel from RGB (216, 210, 197) to (202, 194, 176); the shaded face remains dark (about RGB 55, 55, 50), so night/shadow readability still merits a separate matched check. The new glyphs themselves remain unjudged at normal in-world distance, and resident recognition/repetition has not been observed in ordinary play.

**All-slot/runtime recheck (2026-10-09):** the stone presentation now overrides every mesh material slot; its regression confirms Starter Content `SM_Rock` currently has just one. The UE 5.8.3 scratch build and focused presentation automation passed. A 60-second-capped, real-RHI noon Game capture replaced the blue map proxies and confirmed that collision/navigation remain with the saved proxies, but the stacked forms still read as starkly lit/shadowed rocks. Adding more material-slot handling was not the missing art improvement. Inspect surface response and silhouette separately before further changing the tall landmarks; keep the imported mossy Tripo cairn as a small grounded prop rather than stretching its 103 × 110 × 85 cm pile across the narrow 2.2–3.0 m totems. Details and logs: [`Listening Stones surface override`](findings/2026-10-09-listening-stones-material-override.md).

**First visual milestone:** an old weathered spiral at Listening Stones, a younger transformed descendant elsewhere, and a small derivative motif near a traveled social location. Better stone materials, moss/lichen and careful placement should make lineage visually legible without UI.

**Listening Stones runtime shape (2026-10-09):** the earlier transient stacked-rock forms read as cartoon cairns when fitted to the tall, narrow proxies. Game/PIE now presents one deterministic tapered procedural stone per proxy; the saved cubes still own collision and navigation. A slightly broken cap and a darker slate tint read more like standing stone in the noon capture, but the surface remains smooth and textureless, and the close framing makes the stones dominate. Treat this as a reversible shape prototype. The next art pass should add subtle weathering/lichen and judge the stones at ordinary gameplay distance before calling the landmark finished. See [`Listening Stones surface and procedural form`](findings/2026-10-09-listening-stones-material-override.md).

## 2026-10-08 — A small food-web cue at Tideglass

Minnow breaches make brief surface ripples, and dragonflies can investigate them. Shore crabs share the cue: a nearby minnow splash draws one crab toward the water, while wind and rain impacts still startle it back. Both shore crabs and the minnow school now respond to a nearby grounded, hopping, or perched Raven: a settled approach produces one brief scatter, then rearms only after the bird leaves the outer radius (650 cm for crabs; 800 cm for minnows), with the existing eight-second cooldown. A minnow startle also creates one short ring at the nearest fish, making the Raven's cause visible without turning the school into prey. Low Raven flybys retain their prior cues. Each response is local and temporary; visitor-made ripples remain inert, and no agent request or saved-world state is added. This creates a small, readable cause-and-effect beat among the companion and wild species without turning wildlife into targets or guaranteed interactions. UE 5.8.3 isolated scratch builds succeeded; `CaptiveSky2.Agent.NightEcology` passed with the crab's perched-presence, lingering, state-change-within-radius, and leave/reapproach regressions ([log](../Saved/Logs/Codex_ShoreCrabRavenPresence_20261008.log)), and `CaptiveSky2.Agent.TidepoolMinnows` passed with the minnow's distance/height bounds, scatter direction, one-shot ripple, and leave/reapproach coverage ([log](../Saved/Logs/Codex_MinnowRippleContained_Automation.log)). Both runs disabled agent thinking. These tests verify bounded logic, not its in-world frequency or visual readability; a current close capture shows the ring but ordinary-view readability and naturalness still need a bounded Game/PIE observation.

## 2026-10-08 — A quiet Raven–stag recognition

The existing stag perception now has an explicit Raven integration regression: a close, calm, perched Raven fixture with an `AgentBrainComponent` triggers the stag's existing brief look-around, while a low Raven flyby still startles it. The quiet cue leaves the stag in place and makes no model request. The UE 5.8.3 scratch target built successfully and `CaptiveSky2.Agent.WoodlandDeer` passed with NullRHI, Python/thinking disabled, zero model requests, and isolated world data ([log](../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenStagNotice_20261008_retry.log)). This adds coverage for an existing behavior, not a new autonomous social routine; in-world frequency and visual readability still need a bounded Game/PIE observation.

## Character inspirations — Sierra and Quest for Glory

The user would like future characters that borrow the spirit of old Sierra games,
especially Quest for Glory: a very intelligent rat (the exact reference/name is
uncertain; “Erasmus” was floated as a possibility), some of the wizard
characters, and perhaps the innkeeper archetype. Treat these as tonal and role
inspirations rather than direct reproductions. Any eventual Island characters
should have original names, histories, motivations, and relationships; keep
their arrival and roles open until the user chooses which references matter
most. The existing Innkeeper need not be replaced or duplicated just to satisfy
the inspiration.

## Concept art — The first crossing becomes a place

Aspirational visual target rather than a proposal to immediately construct a complete settlement. The useful hierarchy is intimate resident activity in the foreground, infrastructure connecting people through the middle ground, architecture/ecology beyond, and geography holding everything together.

**Reproducible direction / prompt:**

> Wide cinematic 16:9 Unreal-quality view from a slightly elevated footpath above a sheltered Island inlet in late-afternoon golden light. Foreground left: Aster at a compact working bake area integrated into a useful shelter, handling fresh bread; nearby Rowan, a human maker, pauses beside practical tools and a modest workshop that has clearly evolved through use rather than belonging to a fixed historical style. Midground: a newly completed narrow bridge crosses clear water; its structure combines locally sensible stone footings with precisely made timber/metal connections chosen for function rather than “medieval” styling. Two residents cross it and a subtle worn path/cleared verge shows that navigation changed after construction. Across the water, the existing inn has become materially convincing but remains small and hospitable, with warm interior light and a tended hearth. Include persistent stone arrangements near the path, one older and mossed and one newer variation, as quiet evidence of cultural transmission. Vegetation is compositionally zoned: reeds/moist plants at water, maintained verge near buildings, flowering meadow pockets, dense spruce/woodland edge beyond. Clear water, rock shelves and distant mountain/island geography give depth. Materials are tactile—wet stone, weathered timber, plaster/mineral surfaces, metal fittings, grasses, bread and water—with no imposed historical or futuristic genre. Atmospheric perspective, soft cloud shadows, light wind in foliage, tiny human-scale activity, raven overhead. Mood: timeless, humane, alive and beautiful because everything visible has a reason and a history. Avoid fantasy ornament, cyberpunk cues, generic medieval dressing and pristine theme-park perfection. Make the bridge the visual connector between Aster/Rowan in the foreground and the shared social space beyond: simulation history has literally reshaped the landscape.

### Design note

Authored content is scaffolding, not sacred. Prefer removing, simplifying or handing authored elements back to simulation when they do not justify their existence. Simulation-produced features should generally disappear through causal world processes rather than developer cleanup.

### Rhodo material compile diagnosis (2026-10-08)

The currently open `/Game/Plants/Materials/OriginalFlower/MM_Rhododendron__Everestianum__HD` logs two SM6 graph errors: `StaticSwitchParameter` is missing its A input, and `MakeMaterialAttributes` reports an error on Normal. Unreal also reports the base material and its instances falling back to Default Material in Game. The editor still shows two unsaved items, so this inspection did not save, close, or alter the open asset. This is separate from the `dotnet.exe` application-error dialog. After the current editor work is preserved, repair and validate the material in a coordinated Content change before judging this species' appearance.

### Bounded Game highlight retry (2026-10-08)

The isolated scratch Game launch was first stopped by the machine's read-only installed DDC; retrying with `-DDC-ForceMemoryCache -NoZen` passed DDC and initialized D3D12, but did not reach `/Game/Maps/Island`. Startup remained at `Launching UnrealBuildTool ... -Mode=ValidatePlatforms` (Turnkey); its UE-bundled .NET 10 process stayed at 0.14 CPU seconds for roughly 30 seconds, and `AutoSDKInfo.txt` was not refreshed. Only the scratch Game process and its .NET child were stopped. No world session, screenshot, or model request occurred, and the open editor/material were left untouched. A later headless UE commandlet log recorded `UBT AutoSDK ReturnCode: -532462766`, the same 32-bit code as the user's `0xE0434352` dialog; this ties at least one occurrence to UBT's AutoSDK startup path but does not reveal the underlying managed exception. See the [Unreal MCP/Turnkey finding](findings/2026-10-07-unreal-mcp-session.md#2026-10-08-ubt-autosdk-exception-code-correlation). Log: [`Codex_CappedHighlights_20261008_retry.log`](../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_CappedHighlights_20261008_retry.log). Next: investigate the UBT/Turnkey access failure, then resume the bounded highlight capture.

## 2026-10-08 — The raven attends to rainwater

When settled near the Listening Stones, the raven now gives the rain basin one brief glance once its water reaches the existing leaf-floating threshold. It aims at the actual water surface, does not move or change saved state, and cannot repeat the cue while that basin stays wet; a dry interval rearms it. Flying, resting, distant, or visually occluded ravens ignore it. Higher-priority chimes, minnows, dew, wind motes, crab activity, and resident presence retain their attention ordering. No model turn or request was added. The UE 5.8.3 scratch editor target rebuilt successfully, and `CaptiveSky2.Agent.RavenPerch` passed with NullRHI, resident thinking and Python disabled, zero model requests, and isolated world data ([log](../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenRainBasinAttention_verify.log)). The regression covers dry and wet states, grounded versus flying, head movement without body movement, one-shot behavior, and rearming after a dry spell. This verifies the bounded cue, not whether the bird naturally encounters or notices the basin in ordinary play; a short visual observation remains useful.

## 2026-10-08 — Aster notices the Raven's foraging clue

When a visible raven comes within 3.5 m carrying the small twig bundle at its beak, nearby residents receive that concrete observation in their next already-scheduled situation prompt. The wording reports only what can be seen and explicitly leaves the raven's intention unknown; it does not issue a movement target, add memory, or trigger a model call. The Raven can continue or change its own plans independently. The `CaptiveSky2.Agent.RavenPerch` regression checks the close cue at 3 m, its absence just beyond range while broader raven activity is still visible, and suppression by solid occlusion. Real-RHI screenshots show the corrected bundle is readable nearby but easy to miss by 6 m, so residents do not narrate it from a distance. This makes Aster's possible curiosity about the companion better informed without assigning a response. See [`2026-10-08-aster-visible-raven-foraging.md`](findings/2026-10-08-aster-visible-raven-foraging.md).

## 2026-10-09 — Aster notices the Raven's momentary attention

When a visible, settled Raven briefly turns its head toward a chime, rainwater, wildlife, or another already-modeled cue, a resident within 4 m can notice that the bird is attending to something without being told what or why. The fact is included only during the existing attention pose; it does not move either character, write memory, or create another model request. This lets Aster's curiosity respond to a nearby companion gesture while leaving the Raven's motive and next action its own. A real-RHI Island capture shows the pose at 3 m, but the Raven's head turn is subtle at this camera scale, so the cue stays deliberately close-range; the 5 m regression case does not expose it. The UE 5.8.3 editor build and isolated `RavenPerch` automation passed with resident thinking and Python disabled, zero model requests, and isolated world data ([4 m automation log](../Saved/NavBoundsTest/Project/Saved/Logs/RavenAttentionCueAutomation4m.log), [visual capture log](../Saved/NavBoundsTest/Project/Saved/Logs/RavenAttentionPoseSceneCapture.log), [3 m attention frame](../Saved/Viewpoints/RavenWingMotion_/20261009_084614/03_ResidentAttention.png)). The build also exposed and fixed a unity-build collision between two presentation-file-local cube path constants.

## 2026-10-09 — The Raven returns the stag's quiet look

When a settled Raven is within 5 m and clear sight of a stag's existing calm, temporary look-around, the Raven gives one short head glance back. It is only a nonverbal acknowledgment: no movement, memory/world-state write, or model request; existing attention cues keep priority, and the glance cannot repeat while they remain in the same local group. Moving beyond 7 m rearms it for a later reunion. The UE 5.8.3 scratch build succeeded and isolated `CaptiveSky2.Agent.WoodlandDeer` passed with NullRHI, thinking/Python disabled, zero model requests, and isolated world data ([WoodlandDeer log](../Saved/Logs/Codex_RavenStagReciprocal_final_20261009.log)). A `RavenPerch` run loaded the project Raven placeholder and passed an actual head-pivot/rigged-body rotation assertion toward the staged stag ([RavenPerch verification log](../Saved/Logs/Codex_RavenStagRigAttention_verify_20261009.log)). These regressions cover the reciprocal pose, no movement/request, one-shot behavior, and leave/re-approach. A transient UE 5.8.3 real-RHI capture now drives the same nearby-wildlife selection and rendered head turn, verifies the Raven stays in place, and passes `CaptiveSky2.Visual.RavenWingMotion` with thinking disabled and zero model requests ([real-RHI log](../Saved/Logs/Codex_RavenStagGlance_RHI_20261009h.log), [frame](../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Viewpoints/RavenWingMotion_/20261009_105430/10_RavenStagGlance.png)). This is deliberately staged for inspection, including a temporary Raven facing and a transient stag; it confirms that the movement appears in the rig, not that the encounter is naturally framed during play. Visually, the test area is still a blockout: pale/default-looking animal meshes, exposed brown ground, and construction props dominate. It is not a beauty shot. Revisit the encounter in a planted, finished scene after the coordinated landscape/vegetation art pass, then judge it at normal gameplay distance.

## 2026-10-09 — The Raven can forage nest twigs from storm driftwood

Fresh, unturned shore driftwood now offers the Raven an optional close-range source of one small twig bundle when no existing ground-pile source is available. It must land, see and approach the piece; gathering removes that larger driftwood from the shared shore ledger and is persisted before the bundle is granted. Turned discoveries, other wrack types and failed saves remain untouched. The existing Raven shore cue offers the possibility without choosing for the bird; no extra tick or model request is added. UE 5.8.3 build and three isolated wrack/Raven automation cases passed with thinking disabled and zero requests. The tests prove action and ledger behavior, not autonomous travel or natural gameplay framing; those remain the next real-RHI check. See [Raven storm-wrack foraging](findings/2026-10-09-raven-wrack-foraging.md).

## 2026-10-10 — A quiet Raven–fox encounter

Within 5 m and clear sight, a settled Raven can briefly return the look of an
awake fox. The fox pauses, then retreats a short distance within its woodland
patch; resting foxes are left undisturbed. The cue adds no model request,
movement by the Raven, memory, or persistent state. The isolated `RavenPerch`
automation passed with a zero-request cap. The regression is deterministic and
does not yet establish how naturally or visibly this exchange occurs in normal
play; see [the encounter finding](findings/2026-10-10-raven-fox-encounter.md).

## 2026-10-10 — The fox settles near woodland cover

After the first real-RHI view placed the resting fox in an exposed clearing, its
spawn searches outward from the Wind Arch approach for a grounded site 3–14 m
from mature spruce, preferring a sparse grass pocket while keeping at least the
existing 14.5 m landmark clearance. In the live Island it settled 8.15 m from
mature spruce, with a natural opening in the understory; a 1600x900 real-D3D12
frame shows the resting fox clearly. The captured session was bounded at 90
real seconds and zero model requests. On this RTX 4080 Laptop GPU, a matched
200-frame Game sample averaged 73 FPS (P95 15.09 ms), above the 30-FPS floor;
PIE and target-hardware performance still need checking. Details and evidence:
[woodland fox finding](findings/2026-10-10-woodland-fox.md).

## 2026-10-10 — A quiet resident can meet the woodland fox

An awake fox now briefly notices a nearby resident moving at no more than
1.8 m/s when the resident is within 6 m and visible. It uses its existing
look-then-retreat response, remains inside the same 6.5 m home patch, and
does not repeat while the local resident group stays within 8.5 m. Resting and
waking foxes remain undisturbed. This is a short wildlife reaction, not speech
or new conscious agency: it adds no memory, world state or model request.
`CaptiveSky2.Agent.WoodlandFox` checks the cue, one-shot/rearm behavior, and
cooldown. While the glance is active, a nearby resident's existing situation
summary now reports the brief look but explicitly avoids inferring intent; the
fact disappears when the look ends and does not add memory or a thought request.
Rendered in-world timing and framing still need a bounded Game view.

## 2026-10-10 — A clearer quiet glance on the imported Crow

The imported Crow's short attention pose can now turn up to 18 degrees instead
of 12; the procedural Raven keeps its 20-degree range. Timing, priority,
movement and model-request behavior are unchanged. The UE 5.8.3 real-D3D12
`CaptiveSky2.Visual.RavenWingMotion` capture passed and checks the imported
Crow's turn while the actor stays still; the bounded headless
`CaptiveSky2.Agent.RavenPerch` regression passed too, with zero model requests.
The close capture is still framed against the pale Crow/mannequin blockout, so
normal-distance readability in the planted world remains to be judged. Details:
[attention readability finding](findings/2026-10-10-raven-attention-readability.md).

## 2026-10-10 — A quiet fox–stag moment

When an awake fox is visible within 7.5 m, a grazing stag now gives one short
look-around instead of chasing or fleeing. The cue is local and one-shot until
the fox leaves a 10.5 m area, with an 18-second cooldown; a resting/waking fox
or a resting, waking, moving, or otherwise occupied stag is left alone. A
nearby resident who can see both animals may notice the stag briefly looking
toward the fox, while the prompt explicitly leaves its reason and next action
unknown. No speech, memory, persistent state, or model request is added. UE
5.8.3 scratch build and `CaptiveSky2.Agent.WoodlandDeer` passed with NullRHI,
thinking disabled, zero model requests, and a 60-second watchdog. A standalone
Game-context D3D12 capture now shows the runtime stone presentation with Aster
witnessing the glance; its 30-FPS gate held 36.24 FPS for five seconds. The
frame is staged and still includes pale placeholder-looking stones at lower
left, so it proves runtime composition and renderability, not natural encounter
frequency or a polished highlight. The earlier Editor-context frame showed
saved proxy geometry and is not representative of runtime stones.
Details: [fox–stag awareness finding](findings/2026-10-10-fox-stag-awareness.md).

## 2026-10-10 — The Raven notices a nearby dragonfly

A settled Raven can briefly turn its attention toward a visible dragonfly
within 10 m and a modest height band. It does not chase or move, and the
dragonfly's flight is unchanged. The cue is one-shot while the insect remains
in the nearby group and rearms after it leaves; closer existing wildlife cues
and higher-priority attention still win. UE 5.8.3 editor build and the
`CaptiveSky2.Agent.RavenPerch` regression passed with NullRHI, Python disabled,
an isolated world-state root, and a zero model-request cap. AutoSDK validation
returned 0 on this approved run. A real-D3D12 `CaptiveSky2.Visual.RavenWingMotion`
editor-world capture also rendered the staged pair with thinking disabled and
zero model requests ([frame](../Saved/Viewpoints/RavenWingMotion_/20261010_104406/11_RavenDragonflyGlance.png)).
It is only a presentation check: both actors fit, but the pair is off-center,
Raven's pale placeholder body and the thin brown ground remain obvious, and
normal-play frequency/readability are unverified. Reframe after the current
vegetation/material pass and revisit in bounded Game/PIE play. See
[Raven–dragonfly attention](findings/2026-10-10-raven-dragonfly-attention.md).

## 2026-10-10 — Tideglass gameplay habitat baseline

A bounded UE 5.8.3 real-D3D12 Game capture at 11:00 used an isolated data
root, a warmed playtest-local DDC, thinking disabled, and zero model requests.
The Island became ready in 25.8 seconds; the session ended normally after
90.3 seconds. In the wide Tideglass view, the pond edge, fish, dragonflies,
grazing stag, and flying birds appeared together; the stag changed pose over
the captured sequence. This is rendered Game evidence of the existing light
ecology, not a model-driven agency sample.

The same view's 300-frame CSV profile measured 15.95 ms mean frame time and
19.66 ms p95 (about 51 FPS p95) on this RTX 4080 Laptop GPU, above the 30-FPS
floor for this one composition. A close water-edge view measured 25.18 ms p95
(about 40 FPS), also above the floor. These short, fixed-view samples are not
player traversal or target-hardware guarantees. The close frame is nearly
walled by reeds; the wider frame reads as a wet edge, but the far brown ground
and thin tree line remain sparse, and the inn edge and tall stone proxies still
look like blockout. Continue the ecological composition/material pass, then
repeat the wide Game profile and separately test player traversal.

Evidence: [wide gameplay frame](../Saved/Playtests/Codex_WorldLifeReview_20261010/Screenshots_TideglassOverlook/007_Habitat__Overlook.png),
[close-edge frame](../Saved/Playtests/Codex_WorldLifeReview_20261010/Screenshots_Tideglass/004_Life__Closeup.png),
[wide-view CSV profile](../Saved/Profiling/CSV/Profile%2820261010_063512%29.csv),
[close-view CSV profile](../Saved/Profiling/CSV/Profile%2820261010_063135%29.csv),
[wide-view Game log](../Saved/Playtests/Codex_WorldLifeReview_20261010/WorldLifeReview_TideglassOverlook.log).

**Matched stationary Game repeat (2026-10-10, 11:00 Tideglass):** a later
1600x900 real-D3D12 run used the same five-entry Tideglass motion probe (all
five views are identical), an isolated world-data root, a playtest-local DDC,
thinking disabled, zero model requests, and the 120-second cap. After the
15-second profile delay, 300 frames averaged 19.08 ms; p50 was 15.56 ms and
p95 28.24 ms (about 35 FPS at p95), narrowly above the 30-FPS floor for this
one stationary Game composition. Thirteen of 300 frames exceeded 33.3 ms and
the maximum was 181.85 ms; the largest sample is render-thread-heavy, so the
p95 does not hide a fully smooth experience. This is not PIE, traversal, or a
target-hardware guarantee. The repeat confirms the close pool life and foreground
cover, while showing the same exposed brown far ground, thin tree row, cropped
inn, and blockout standing stones that the landscape/material pass should
address. Do not infer that the later profile regressed from the earlier sample:
they are separate sessions and the hitch cause is not yet identified.

Evidence: [later Game frame](../Saved/Playtests/Codex_TideglassNaturalGame_20261010/Screenshots/002_Tideglass.png),
[later Game log](../Saved/Logs/Codex_TideglassNaturalGame_20261010_retry.log),
[300-frame CSV](../Saved/Profiling/CSV/Profile%2820261010_105710%29.csv).

## 2026-10-10 — Listening Stones gather a little lichen

The three transient standing stones now carry six broader, irregular deep-olive
lichen patches each, wrapped around the full surface and sampled against the
same triangles as the rock mesh. This adds a small weathering cue while
leaving the saved proxies, collision, navigation, and persistent world state
untouched. The UE 5.8.3 build and focused presentation automation passed; a
35-second D3D12 Game capture at a close approach shows a lichen patch on the
near stone, with zero model requests. A matched 9 m D3D12 journey view shows
the marks as quiet mottling rather than a prominent decal; its 60-second run
also ended cleanly with zero requests. An editor journey-view retry only
rendered the saved stone proxies and hit a separate existing foliage-count
assertion; the first cold standalone Game launch timed out, but a warmed retry
reached the world and captured the view. Details and evidence:
[transient lichen finding](findings/2026-10-09-listening-stones-material-override.md#transient-lichen-weathering-2026-10-10).

## 2026-10-10 — Tideglass wet-edge variety and layered-cover screen

A one-shot editor-world preview confirmed 7 Typha + 3 Phalaris at the Tideglass
edge and three transient daylight dragonflies. The broad view's full-cover p95
was 28.45 FPS, below the 30-FPS screen; meadow/ground plants alone reached
55.50 FPS p95 and woodland/understory alone 58.11 FPS. The combined
meadow+trees and meadow+understory editor screens reached 56.05 and 44.54 FPS
p95 respectively. All partial screens passed, unlike full cover, but the
measurements were sequential editor SceneCaptures with Blender still open and
do not establish a Game/PIE regression or identify a cause. The current view
still has an exposed brown mid-distance, blockout pool/landmarks, and a
placeholder-looking close dragonfly. Next: repeat full cover and
meadow+understory as matched standalone Game and PIE profiles with other
graphics workloads closed, then inspect a GPU trace before changing counts or
materials. See the [full capture and limits](findings/2026-10-10-tideglass-layer-performance.md).

**17:00 standalone Game visual recheck (2026-10-10):** four bounded frames at
the same Tideglass spectator camera show the actual golden-hour pool, a moving
stag, and active dragonflies. The late frame also confirms the continuing
composition problem: tangled foreground, open brown mid-distance, sparse thin
tree silhouettes, and conspicuous stacked-stone proxies. Treat this as visual
evidence only—the 90-second run had no CSV, PIE traversal, or resident thinking
(zero model requests). It reached the Island only on an elevated retry after a
sandboxed launch stalled in Turnkey; this does not explain the recurring .NET
dialog. The Game log reports 15 foliage/prop materials falling back to Default
Material because their `InstancedStaticMeshes` usage flag is missing. Next:
coordinate the imported-material fix with the Content owner, then build habitat
depth with distinct mid-distance woodland groups and a clear pool approach;
recheck the same Game view and PIE separately. See the [capture notes and
limits](findings/2026-10-10-tideglass-layer-performance.md#bounded-1700-standalone-game-visual-recheck-2026-10-10).

**Instanced StarterContent material repair (2026-10-10):** the actual Island
Game log showed `M_Rock` and `M_Wood_Oak` falling back to Default Material on
HISM instances. UE 5.8.3 compiled their `InstancedStaticMeshes` permutations
without errors; both flags were saved after byte-verified backups. This should
restore authored rock/wood surfaces where those materials are instanced, but
the Game effect remains unverified because a live UnrealEditor process was
present after the save. The 13 Everestian rhododendron fallbacks remain a
separate broken graph and were not toggled. Next: once the editor closes, repeat
the capped zero-request Tideglass Game view and verify both warning removal and
appearance. Details and backups: [material repair finding](findings/2026-10-10-starter-instanced-materials.md).

## 2026-10-31 — Rat's fallen-wood hollow

Rat's first-home concept is a hidden fallen-wood hollow. A local V7 asset
candidate now reads as a horizontal trunk in an oblique Blender view, with an
irregular root canopy and low side roots. It is still too stylized and has not
been reviewed against Island terrain, woodland cover, or Rat-scale movement.
Keep the canonical home and memory unset until a disposable Unreal placement
passes entrance visibility, ground contact, collision, and navigation review.
See [the V7 candidate finding](findings/2026-10-31-rathollow-hidden-v7.md).
