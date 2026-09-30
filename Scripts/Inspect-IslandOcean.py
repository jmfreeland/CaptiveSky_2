"""Read-only Unreal Editor diagnostic: the Island's ocean actor, its material, and M_SimpleOcean."""

import traceback
import unreal


def prop(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def main():
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Island")
    world = unreal.EditorLevelLibrary.get_editor_world()
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log("[Ocean] Level actors: {}".format(len(actors)))
    for actor in actors:
        name = actor.get_name()
        label = actor.get_actor_label()
        if any(key in (name + label).lower() for key in ("ocean", "water", "sea")):
            loc, scale = actor.get_actor_location(), actor.get_actor_scale3d()
            unreal.log("[Ocean] Actor {} / {} [{}] loc=({:.0f},{:.0f},{:.0f}) scale=({:.2f},{:.2f},{:.2f})".format(
                name, label, actor.get_class().get_name(), loc.x, loc.y, loc.z, scale.x, scale.y, scale.z))
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                mesh = comp.static_mesh
                unreal.log("[Ocean]   mesh={} bounds={}".format(mesh.get_path_name() if mesh else None,
                                                               mesh.get_bounds() if mesh else None))
                for index in range(comp.get_num_materials()):
                    mat = comp.get_material(index)
                    unreal.log("[Ocean]   slot {} material={} [{}]".format(
                        index, mat.get_path_name() if mat else None, mat.get_class().get_name() if mat else None))
    # Landscape extents for scale.
    for actor in actors:
        if actor.get_class().get_name().startswith("Landscape") and actor.get_class().get_name() == "Landscape":
            origin, extent = actor.get_actor_bounds(False)
            unreal.log("[Ocean] Landscape bounds origin=({:.0f},{:.0f},{:.0f}) extent=({:.0f},{:.0f},{:.0f})".format(
                origin.x, origin.y, origin.z, extent.x, extent.y, extent.z))

    material = unreal.load_asset("/Game/Materials/M_SimpleOcean")
    if material:
        unreal.log("[Ocean] M_SimpleOcean domain={} blend={} shading={}".format(
            prop(material, "material_domain"), prop(material, "blend_mode"), prop(material, "shading_model")))
        for expression in unreal.MaterialEditingLibrary.get_used_textures(material) if hasattr(unreal.MaterialEditingLibrary, "get_used_textures") else []:
            unreal.log("[Ocean]   texture {}".format(expression.get_path_name()))
        try:
            exprs = unreal.MaterialEditingLibrary.get_material_expressions(material) if hasattr(unreal.MaterialEditingLibrary, "get_material_expressions") else []
        except Exception:
            exprs = []
        for e in exprs:
            unreal.log("[Ocean]   expr {}".format(e.get_class().get_name()))
    mpc = unreal.load_asset("/Game/Materials/MPC_IslandEnvironment") or unreal.load_asset("/Game/Environment/MPC_IslandEnvironment")
    unreal.log("[Ocean] MPC loaded: {}".format(mpc.get_path_name() if mpc else None))


try:
    main()
except Exception:
    unreal.log_error("[Ocean] " + traceback.format_exc())
