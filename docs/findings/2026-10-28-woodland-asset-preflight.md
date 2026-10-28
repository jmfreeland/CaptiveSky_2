# Woodland Megaplant asset preflight — 2026-10-28

The current `IslandWeather.cpp` references four Megaplant skeletal packages for
the mixed woodland: Norway Maple, European Aspen, European Beech, and Black
Alder. All four corresponding `.uasset` package files are present in this
project's local `Content/Megaplant_Library` directory. The source also retains
the existing spruce and two procedural coppice meshes; this check does not
measure how many instances of any species are actually placed.

I added `Scripts/check_island_tree_assets.py` as a read-only preflight. It reads
the current C++ references and checks that each expected package exists on disk,
so a missing import is visible before opening the editor. The three focused
Python tests cover path extraction, package mapping, and rejecting unrelated
asset paths.

This is strictly a disk-presence check. It does **not** prove Unreal can load
the packages, that their materials or wind data work, that the species are
visible in the live Island, or that Nanite foliage improves memory or frame
time. Those are still the necessary editor/runtime gates. Unreal was not
running during this audit, so no project launch or map change was attempted.

Next: when Unreal is open again, run the existing woodland population audit and
inspect a wide game view. Confirm species counts and material/wind appearance,
then profile frame time and memory before changing scatter density or adding
more tree assets.
