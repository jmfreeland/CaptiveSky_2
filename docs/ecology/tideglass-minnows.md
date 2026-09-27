# Tideglass Minnows

The shallow Tideglass pool now has one small, visual-only minnow school during the Island day (06:00–19:00). `AIslandWeather` owns its lifetime alongside the other habitat routines; repeated refreshes reuse the same school, dusk removes it, and dawn creates one again. The five silver placeholder fish have no collision and are not movement targets.

The fish trace a compact loop and tighten their spread a little during heavy rain. A nearby visitor or resident may quietly watch them: the school fans away for a couple of seconds, then returns to its local path. This response changes no saved state, transfers no ownership, and makes no model requests. Residents only receive it as optional nearby wildlife context, and may ignore it entirely.

This is a habitat prototype, not a simulation of tide, feeding, reproduction, or water quality. It uses the pool marker as its center and does not infer depth or safe habitat from the marker. The silver spheres are temporary placeholder art. `CaptiveSky2.Agent.TidepoolMinnows` covers the bounded population, local movement, scatter/regroup response, visible visitor interaction, resident inspection, rain response, and day/night lifecycle without a live play session.
