# Readable attention on the imported Crow rig

The imported Crow now turns up to 18 degrees toward a nearby attention target
(previously 12 degrees). Cue duration, priority, Raven movement, and request
behavior are unchanged; the procedural fallback retains its existing 20-degree
limit. This makes the imported rig's quiet acknowledgment less constrained
without turning a glance into a full-body reaction.

## Verification

- The UE 5.8.3 scratch editor target built and linked successfully.
- `CaptiveSky2.Visual.RavenWingMotion` passed with D3D12 and saved real-RHI
  frames. Its Aster-facing fixture asserts that the imported Crow turns between
  12 and 18 degrees while the Raven actor stays in place. The transient
  stag-attention fixture also passed. Thinking was disabled, the model-request
  cap was zero, and the play-session watchdog was 60 seconds.
- `CaptiveSky2.Agent.RavenPerch` passed separately with NullRHI and the same
  zero-request/60-second safeguards.
- The visual test still expected the carried twigs to be an instanced-static
  mesh, but the current Raven builds them as a procedural mesh. Its fixture now
  validates the actual procedural section, non-empty tapered geometry,
  collision/navigation isolation, and its beak-relative bounds. This changes
  test coverage only, not the runtime twig asset.

The captures are reproducible close-ups in the current blockout scene, not a
beauty shot or an ordinary untended encounter. The pale Crow and mannequin
remain visually dominant. A rendered in-world observation is still needed to
judge whether the stronger glance reads naturally at normal gameplay distance.

Evidence: `Saved/CompileScratch/Claude_Props/Saved/Logs/RavenAttentionReadability.log`
and `Saved/CompileScratch/Claude_Props/Saved/Viewpoints/RavenWingMotion_/20261010_052806/03_ResidentAttention.png`.
