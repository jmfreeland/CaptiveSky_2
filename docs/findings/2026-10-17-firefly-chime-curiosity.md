# Firefly curiosity at Listening Stones (2026-10-17)

The single nearby night firefly now gives an audible Listening Stones chime a
small, readable answer. It drifts up to 55 cm toward the sound source along a
1.8-second ease-in/ease-out arc, then resumes its ordinary home-centered flight.
Its dim-phase glow lifts by 25% during the response and fades with it. The
existing acoustic-radius check and per-chime deduplication remain in force.

The response is local and transient: no new insects or lights are spawned, no
collision or navigation state changes, and no resident/model request or
persistent world-state write is involved. The movement still passes through the
firefly's existing swept path resolver.

Validation: the UE 5.8.3 editor target built and linked in the content-bearing
`Saved/CompileScratch/Claude_Props` project. The standalone
`CaptiveSky2.Agent.FireflyChimeCuriosity` automation passed under NullRHI with
agent thinking disabled and zero model requests
(`Saved/CompileScratch/Claude_Props/Saved/Logs/FireflyChimeCuriosity_Final_20261017.log`).
It checks source direction, deduplication, the bounded approach arc, return to
the ordinary path, and the glow lift. A separate broader
`CaptiveSky2.Agent.ListeningStonePresentation` run still fails four existing
visitor-controller assertions in its synthetic player setup; its log contains
no failure from the firefly-specific assertions. In-world visual readability
remains pending because the editor is not currently running.
