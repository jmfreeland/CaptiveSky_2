# The Raven's nest has a woven floor (2026-10-09)

The persistent nest's earlier placeholder weave consisted only of seven short
twigs per layer around the rim. A five-twig, two-course lattice now sits beneath
the first ring, so the silhouette reads more like a shallow bowl than a hollow
stack of hoops. Its slight offsets and angles are seeded from the nest site, so
rebuilding or restoring the same nest keeps the same appearance.

The base is part of the existing visual-only instanced mesh. It adds no
collision, navigation, shelter, or safety semantics, and it changes no saved
world-state fields. A storm that removes the lowest layer also removes the base;
the next weave restores it naturally with the remaining nest.

`CaptiveSky2.Agent.IslandNest` now checks the base's visible instance count,
low/flat placement, deterministic reconstruction, and the existing alternating
rim courses with the added base offset. Both edited translation units compiled
successfully with the cached UE 5.8.3 response configuration and VS 18. This was
compile-only: no UHT pass, module link, or automation run was performed, so the
new behavior is not yet runtime-validated. A visual capture is still needed to
judge the placeholder geometry in the live Island.
