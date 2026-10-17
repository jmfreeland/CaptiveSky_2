# Rat Hollow RootArch V2 candidate

## Purpose

Create a reversible, Rat-scale ground-entry version of the existing fallen-log
hollow. The established `RatHollowRootArch` asset is left untouched; V2 is a
separate source/export candidate and has not been imported into the Island.

## Candidate

- Source: `Scripts/AssetPipeline/assets/rat_hollow_rootarch_v2.py`
- Export: `Saved/CaptiveSky/ComfyBlender/RatHollowRootArchBurrow/`
- Overall dimensions: 192 x 118 x 89 cm; approximately 924 triangles.
- Materials: bark, hollowwood, moss, dirt, and straw.
- Eight simple UBX colliders define two side walls, a roof, four overlapping
  floor sections, and a rear stop. The passage is 56 cm wide, with floor tops
  rising from 4 cm to 16 cm in 4 cm steps. Static tests check a 16 x 24 cm Rat
  capsule at the center of each ramp section.
- The entrance threshold anchor is 3 cm above the asset origin. The broken rim
  removes six lower annular sectors to merge the opening with the ground.
- The 22.5 x 16.8 cm bedding patch is enlarged and seated on the lowered ramp;
  it remains shallow enough to step over and does not narrow the 56 cm collider
  corridor.

## Validation

- Four source-level unit tests pass with Blender 5.2's bundled Python.
- Blender 5.2.2 exports the FBX; `verify_asset.py` reimports it and reports no
  problems, one UV layer, five material slots, and all eight collision boxes.
- The front/three-quarter preview confirms the lower bark rim is open. It is a
  modeling review only; the plain interior floor still needs contextual review.

## Not yet validated

The candidate has not been imported into Unreal, placed in the level, checked
against the Rat's actual capsule/navmesh, or tested in play. UnrealEditor PID
22688 was left running at the user's request, so do not attempt another editor
launch until that session is available for a safe paired test. The asset remains
reversible and separate from the existing Island object.
