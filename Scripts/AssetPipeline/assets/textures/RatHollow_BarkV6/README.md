# RatHollow V6 bark texture candidate

This versioned texture set is used only by `rathollow_barkv6.py`; it does not
replace the shared Bark or RatHollow V4 maps. The build script copies these
files into the local `Saved/CaptiveSky/ComfyBlender/Library/Textures/` folder
only when each destination is absent, and refuses a same-name file with a
different hash.

`T_Lib_RatHollow_BarkV6_BC.png` was generated with the built-in image-generation
tool from the earlier RatHollow bark albedo. The prompt asked for a seamless,
muted charcoal grey-brown fallen broadleaf bark tile with lower contrast,
broken bark plates, shallow fissures, and no lighting, moss, or focal feature.
The normal and ORM maps are versioned copies of the V4 maps, not fresh PBR
derivatives of this image. Treat this as a material-comparison prototype and
regenerate those maps before production use.

Blender 5.2.2 currently exports the V6 2,352-triangle mesh and verifies the
FBX with five box colliders, one `UVMap`, and `PROBLEMS []`. Its neutral
entrance-facing preview still reads as a stylized, nearly circular stump; it
has not been imported into Unreal, reviewed in woodland lighting, placed in
the Island, or approved as the Rat's home.
