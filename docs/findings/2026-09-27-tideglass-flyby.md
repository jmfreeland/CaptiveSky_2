# Tideglass fish response to low raven flight (2026-09-27)

The daytime minnow school already scatters briefly when a resident or visitor chooses to watch it at close range. To let the habitat respond to ordinary movement too, the school checks about three times a second for an embodied raven controller in its `Flying` state. A pass within 5.5 metres horizontally and 1.5–7 metres above the shallows triggers the same brief fan-and-regroup motion. An eight-second cooldown prevents rapid repeat reactions while the raven stays nearby.

This is a small visual reaction only. Minnows remain wild, uncapturable, nonblocking and nonpersistent. The flyby makes no model calls, adds no movement target, and does not change the raven's action or memory. At the current prototype scale, the height and distance gates are deliberately conservative; editor inspection should confirm the trigger feels like a low pass rather than distant proximity.

`CaptiveSky2.Agent.TidepoolMinnows` covers high-flight and distant non-triggers, a low-pass response away from the approach, and regrouping without retriggering during the cooldown, alongside the prior quiet-observation behavior. `CaptiveSky2.Agent.NightEcology` also passes as a regression check. Both tests use brain-free fixtures. The UE 5.8.3 editor build succeeded; in-editor shoreline placement and visual inspection remain outstanding.
