# Shore-crab response to a raven flyby

The two Tideglass shore crabs now react when the embodied raven is actually flying close above them. Each crab checks at a modest interval and only responds to a raven controller in `Flying` state within 4.25 metres horizontally and 1.2–4.8 metres above the crab. One qualifying pass produces the existing 2.4-second scurry away from the raven, followed by the crab's ordinary local movement. An eight-second per-crab cooldown avoids repeated reactions to a raven that circles overhead.

This connects two existing ecology routines without adding another autonomous thought, model request, target, capture mechanic, ownership claim, or persistent world state. It is a visual startle response, not a claim that the crab reaches or finds cover. `CaptiveSky2.Agent.NightEcology` exercises a close low pass, direction away from the raven, high and distant non-triggers, and cooldown suppression in its isolated fixture.

The UE 5.8.3 build and offline automation validate the logic, but the saved Island's crab placement, the raven's real flight path, and whether the height window reads naturally still need rendered PIE inspection. Do not retune the thresholds from the fixture alone.
