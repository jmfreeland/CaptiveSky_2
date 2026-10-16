# Resident world-proposal queue cap (2026-10-16)

The human-review inbox now accepts at most **three pending proposals per
resident**, across both new-object requests and upgrade suggestions. The
existing global 64-request cap and exact-duplicate rejection remain intact.
Accepted or declined proposals no longer count as pending, so a resident gets
capacity back after a human decision. Rejections return an explanatory message
through the normal action result; no model request, asset generation, import,
or world edit is triggered by the cap.

The queue automation fixture now covers a mixed set of new-object and upgrade
proposals, rejection of a fourth pending idea without allocating a request ID,
another resident retaining access to the shared inbox, and an accepted review
freeing the original resident's slot.

## Validation

UE 5.8.3 built and linked the full `CaptiveSky_2Editor Win64 Development`
target from a fresh, unique scratch project output directory. The build ran
173 actions, including `IslandAssetRequestQueue.cpp` and
`IslandAssetRequestQueueTests.cpp`, and completed with `Result: Succeeded`.
Scratch `Source`, `Content`, and `Config` junctions point to the shared project;
generated build outputs and the linked module stayed under
`Saved/CompileScratch/Codex_ResidentRequestQueue_20261016/`. The main editor DLL
was not replaced.

The automation test itself has **not** run yet. Project-opening commandlets
still crash before automation startup with the previously recorded null access
violation (`PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`), and the
interactive editor's lock was respected. Therefore this is compile/link
evidence, not a runtime pass. Once project startup is healthy, run
`CaptiveSky2.Agent.AssetRequestQueue` with resident thinking disabled and zero
permitted model requests.
