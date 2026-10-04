"""Imports a GLB downloaded by Scripts/Tripo.py into /Game/Generated/Tripo/<Name>/ and reports what arrived.

For each Saved/CaptiveSky/Tripo/<Name>/model.glb (all of them, or just TRIPO_ASSET=<Name>) it:
  - rescales it so the longest side is the size given to Tripo.py --size-cm (or TRIPO_SIZE_CM; default: Tripo's 1 m)
    and moves the pivot to bottom-centre, so the prop stands on the ground at its actor location;
  - imports it (Interchange) as a static mesh plus its materials/textures, replacing any earlier import of the same
    name, so a regenerated prop can be re-imported over itself;
  - adds simple collision (TRIPO_COLLISION=box|convex|none, default box) so residents' navigation and physics see
    something cheap rather than the render mesh;
  - writes Saved/CaptiveSky/Tripo/<Name>/import.json (asset paths, vertices, size in cm) and logs the same.

Nothing outside /Game/Generated/Tripo is touched. Content/ is gitignored, so imported props are local-only: keep
meta.json and the GLB (Saved/ is local too) if a prop must be recreated elsewhere.

Run headless in the project (close the editor first, or use a scratch copy):
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file> -unattended -NoZen -abslog=<abs log>
"""

import json
import os
import struct
import traceback
import unreal

DEST_ROOT = "/Game/Generated/Tripo"
SOURCE_ROOT = os.path.normpath(os.path.join(unreal.Paths.project_dir(), "Saved", "CaptiveSky", "Tripo"))
COLLISION = os.environ.get("TRIPO_COLLISION", "box").lower()


def log(message):
    unreal.log("[TripoImport] " + str(message))


def fit_glb(src, dst, longest_cm):
    """Rewrites POSITION data so the longest side is `longest_cm` and the pivot is bottom-centre.

    Tripo returns every model about 1 m on its longest side, centred, so real size and a ground-level pivot have to
    be applied here. glTF is Y-up and in metres; the importer turns 1 m into 100 cm.
    """
    data = open(src, "rb").read()
    json_len, _ = struct.unpack_from("<II", data, 12)
    gltf = json.loads(data[20:20 + json_len])
    bin_len, _ = struct.unpack_from("<II", data, 20 + json_len)
    binary = bytearray(data[28 + json_len:28 + json_len + bin_len])

    accessors = []
    for mesh in gltf["meshes"]:
        for prim in mesh["primitives"]:
            index = prim["attributes"]["POSITION"]
            if index not in accessors:
                accessors.append(index)
    spans = []
    for index in accessors:
        acc = gltf["accessors"][index]
        view = gltf["bufferViews"][acc["bufferView"]]
        spans.append((acc, view["byteOffset"] + acc.get("byteOffset", 0), view.get("byteStride", 12)))

    lo, hi = [1e30] * 3, [-1e30] * 3
    for acc, offset, stride in spans:
        for i in range(acc["count"]):
            p = struct.unpack_from("<3f", binary, offset + i * stride)
            lo = [min(a, b) for a, b in zip(lo, p)]
            hi = [max(a, b) for a, b in zip(hi, p)]
    longest_m = max(h - l for l, h in zip(lo, hi))
    scale = (longest_cm / 100.0) / longest_m
    shift = [-(lo[0] + hi[0]) / 2, -lo[1], -(lo[2] + hi[2]) / 2]

    for acc, offset, stride in spans:
        new_lo, new_hi = [1e30] * 3, [-1e30] * 3
        for i in range(acc["count"]):
            at = offset + i * stride
            p = [(c + s) * scale for c, s in zip(struct.unpack_from("<3f", binary, at), shift)]
            struct.pack_into("<3f", binary, at, *p)
            new_lo = [min(a, b) for a, b in zip(new_lo, p)]
            new_hi = [max(a, b) for a, b in zip(new_hi, p)]
        acc["min"], acc["max"] = new_lo, new_hi

    body = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    body += b" " * (-len(body) % 4)
    out = struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(body) + 8 + len(binary))
    out += struct.pack("<II", len(body), 0x4E4F534A) + body
    out += struct.pack("<II", len(binary), 0x004E4942) + bytes(binary)
    with open(dst, "wb") as handle:
        handle.write(out)
    return longest_m * 100.0


def import_glb(glb_path, dest_dir):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", glb_path)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(p) for p in task.get_editor_property("imported_object_paths")]


def add_collision(mesh):
    if COLLISION == "none":
        return "none"
    library = unreal.EditorStaticMeshLibrary
    library.remove_collisions(mesh)
    if COLLISION == "convex":
        library.set_convex_decomposition_collisions(mesh, 4, 8, 100000)
    else:
        library.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
    return COLLISION


def describe(name, paths):
    meshes = [p for p in paths if isinstance(unreal.load_asset(p), unreal.StaticMesh)]
    if not meshes:
        raise RuntimeError("{}: import produced no static mesh (got {})".format(name, paths))
    mesh = unreal.load_asset(meshes[0])
    collision = add_collision(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    size = box.max - box.min
    vertices = unreal.EditorStaticMeshLibrary.get_number_verts(mesh, 0)
    report = {
        "name": name,
        "assets": paths,
        "static_mesh": meshes[0],
        "vertices_lod0": vertices,
        "size_cm": [round(size.x, 1), round(size.y, 1), round(size.z, 1)],
        "pivot_min_cm": [round(box.min.x, 1), round(box.min.y, 1), round(box.min.z, 1)],
        "material_slots": len(mesh.static_materials),
        "collision": collision,
    }
    return report


def run():
    only = os.environ.get("TRIPO_ASSET")
    names = [only] if only else sorted(d for d in os.listdir(SOURCE_ROOT) if os.path.isfile(os.path.join(SOURCE_ROOT, d, "model.glb")))
    if not names:
        raise RuntimeError("No Tripo models under " + SOURCE_ROOT)
    for name in names:
        folder = os.path.join(SOURCE_ROOT, name)
        glb = os.path.join(folder, "model.glb")
        if not os.path.isfile(glb):
            raise RuntimeError("Missing " + glb)
        size_cm = float(os.environ.get("TRIPO_SIZE_CM") or 0)
        if not size_cm and os.path.isfile(os.path.join(folder, "meta.json")):
            with open(os.path.join(folder, "meta.json"), "r", encoding="utf-8") as handle:
                size_cm = float(json.load(handle).get("size_cm") or 0)
        fitted = os.path.join(folder, name + ".glb")
        fit_glb(glb, fitted, size_cm or 100.0)
        dest = "{}/{}".format(DEST_ROOT, name)
        if unreal.EditorAssetLibrary.does_directory_exist(dest):
            unreal.EditorAssetLibrary.delete_directory(dest)
        paths = import_glb(fitted, dest)
        log("{}: imported {}".format(name, paths))
        report = describe(name, paths)
        report["requested_size_cm"] = size_cm or None
        with open(os.path.join(SOURCE_ROOT, name, "import.json"), "w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=2)
        log("{}: {}".format(name, json.dumps(report)))


try:
    run()
except Exception:
    unreal.log_error("[TripoImport] " + traceback.format_exc())
