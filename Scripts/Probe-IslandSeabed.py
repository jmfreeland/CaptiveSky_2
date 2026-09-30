"""Read-only Unreal Editor diagnostic: landscape height relative to the ocean plane, sampled on a grid."""

import traceback
import unreal

SEA_Z = 940.0
STEP = 12000.0
HALF = 204000.0


def main():
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Island")
    world = unreal.EditorLevelLibrary.get_editor_world()
    ocean = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if a.get_actor_label() == "OceanPlane"]
    rows, depths = [], []
    y = -HALF
    while y <= HALF:
        row = ""
        x = -HALF
        while x <= HALF:
            hit = unreal.SystemLibrary.line_trace_single(
                world, unreal.Vector(x, y, 60000), unreal.Vector(x, y, -20000),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ocean, unreal.DrawDebugTrace.NONE, True)
            if hit is None:
                row += "?"
            else:
                z = hit.to_tuple()[4].z if hasattr(hit, "to_tuple") else None
                z = hit.impact_point.z if hasattr(hit, "impact_point") else z
                depth = SEA_Z - z
                depths.append(depth)
                row += "#" if depth < 0 else ("1" if depth < 100 else "2" if depth < 300 else "3" if depth < 700 else "4")
            x += STEP
        rows.append(row)
        y += STEP
    for row in rows:
        unreal.log("[Seabed] " + row)
    under = sorted(d for d in depths if d >= 0)
    unreal.log("[Seabed] samples={} land={} underwater={}".format(len(depths), len(depths) - len(under), len(under)))
    if under:
        for q in (0.1, 0.25, 0.5, 0.75, 0.9, 1.0):
            unreal.log("[Seabed] underwater depth p{:.0f} = {:.0f} cm".format(q * 100, under[min(len(under) - 1, int(q * len(under)))]))
    unreal.log("[Seabed] legend: # land, 1 <1 m, 2 <3 m, 3 <7 m, 4 deeper, ? no hit")


try:
    main()
except Exception:
    unreal.log_error("[Seabed] " + traceback.format_exc())
