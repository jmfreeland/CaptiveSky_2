# Periwinkle response to a settled Raven

At Tideglass, an attached periwinkle now briefly withdraws its eyestalks when
the Raven lands, hops, or perches within 3 m and within 1.8 m vertically. The
response is checked at 2.5 Hz, latches once per Raven approach, and rearms only
after the bird leaves a 5.2 m horizontal ring. Flight and high overhead passes
are ignored, so ordinary movement does not create constant startle behavior.

The snail remains attached to its home and remains untargetable as a movement
goal. The cue reuses the existing short, reversible observation tuck; it adds
no model calls, persistent state, ownership, or predator/prey mechanic.

## Validation

The UE 5.8.3 isolated `Saved/CompileScratch/Claude_Props` target compiled and
linked successfully after this change. The main editor remains unresponsive
(PID 22688) and was left untouched. An earlier scratch automation launch did
not produce a project/test log, so no runtime pass is claimed. The focused
`CaptiveSky2.Agent.NightEcology` assertions cover a settled approach, no repeat
while the Raven lingers, ignored low flybys and high overhead presence, and
rearming after the Raven leaves; run them when project startup is available.
