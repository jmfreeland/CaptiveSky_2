# Raven nest moss lining — 2026-10-18

Added a small, deterministic green lining to each woven raven nest. Seven
flattened engine-sphere instances form a shallow cup inside the twig weave.
They use one instanced component and a transient tint, cast no shadows, affect
neither collision nor navigation, and are recreated from the nest site tag;
the persistent nest record and build action are unchanged. Reapplying a record
clears and rebuilds the same bounded set rather than accumulating instances.

Validation: the current source snapshot built in an isolated UE 5.8 scratch
project (175/175 actions, including `IslandNest.cpp` and its automation unit).
`CaptiveSky2.Agent.IslandNest` completed with `Result={Success}` under NullRHI,
with agent thinking disabled and the model-request cap set to zero. The test
checks the seven patches, transient tint, no-collision/no-shadow/no-navigation
flags, and idempotent instance count. The scratch project intentionally has no
Island `Content/`, so missing optional foliage asset warnings are expected and
this is not evidence of full-project startup or visual quality.

The main Island map and persistent world state were not changed. Inspect the
nest in the responsive Unreal editor before treating the green lining as a
final art choice; the headless test cannot judge how it reads among the actual
wood materials, perch, and surrounding foliage.
