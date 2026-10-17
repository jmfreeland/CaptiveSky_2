# Sparse hazel thickets at the woodland edge — 2026-10-18

Added a new HISM woodland-edge layer using the already imported local
`WoodlandHazelThicket` mesh. The existing asset import report records a
193 x 187 x 234 cm static mesh at approximately 5,778 triangles, with four
material slots and no collision shapes. Its package is local/generated content
under `/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012`; it
is not tracked by Git, so other checkouts without that imported package will
gracefully omit this optional accent.

Eligible spruce groves are selected by a stable world-seed hash at roughly one
in eight. Each selected grove can receive at most one thicket, chosen in a
23.5–33 m edge ring with bounded landscape traces, a 26 m landmark clearance,
and spacing checks against nearby trees, shrubs, flowers, and other thickets.
The HISM has no collision, navigation, or shadow contribution and is culled
between 100 and 240 m. The selection helper has a deterministic distribution
automation test. This is an additive, reversible woodland-composition change;
it does not modify the saved Island map or persistent world state.

UE 5.8.3 UnrealHeaderTool and the full editor-target build passed in the
existing isolated `Claude_Props` scratch project; this also compiled the new
automation test and linked its scratch DLL without replacing the editor-held
main-project DLL. A direct main-project link was blocked by that DLL lock, so
the editor process was left alone. The focused test runner did not produce a
project log, and the most recent project NullRHI launch reproduced the known
zero-second null-access startup crash (`PCallStackHash
F405610C114D120D4B6F5ABA901EC469AC9FAA26`); its transcript and context are in
`Saved/Crashes/_0000/`. Do not claim the test or Game behavior passed until a
symbol-capable startup diagnosis allows a controlled run. Next: run the focused
automation and capture the same wide Tideglass Game view, then measure p95 and
confirm the new broadleaf layer improves the exposed mid-distance without
lowering the established 30-FPS floor. No Island map or persistent world state
was modified.
