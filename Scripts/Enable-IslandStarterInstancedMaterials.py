"""Enable instanced-static-mesh usage on two verified StarterContent materials.

Run in UE 5.8.3 with -ExecutePythonScript=<absolute script path>. The script
backs up both source packages under Saved/AssetBackups before editing, compiles
all required permutations before saving either package, then verifies the saved
usage flags. Loading the main project may load its default Island map, but the
script never changes or saves that map.
"""

import hashlib
import os
import shutil
import traceback

import unreal


MATERIALS = (
    ("/Game/StarterContent/Props/Materials/M_Rock", "StarterContent/Props/Materials/M_Rock.uasset"),
    ("/Game/StarterContent/Materials/M_Wood_Oak", "StarterContent/Materials/M_Wood_Oak.uasset"),
)
BACKUP_DIRNAME = "Codex_StarterInstancedMaterials_20261010"


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def backup_package(source_path, backup_path):
    source_hash = sha256_file(source_path)
    if os.path.exists(backup_path):
        backup_hash = sha256_file(backup_path)
        if backup_hash != source_hash:
            raise RuntimeError(
                "Existing backup differs from source; refusing to overwrite it: {}".format(backup_path)
            )
    else:
        os.makedirs(os.path.dirname(backup_path), exist_ok=True)
        shutil.copy2(source_path, backup_path)
        if sha256_file(backup_path) != source_hash:
            raise RuntimeError("Backup verification failed: {}".format(backup_path))
    return source_hash


def main():
    project_content = unreal.Paths.project_content_dir()
    backup_dir = os.path.join(unreal.Paths.project_saved_dir(), "AssetBackups", BACKUP_DIRNAME)
    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    library = unreal.MaterialEditingLibrary
    packages = []

    # Back up and preflight every package before changing any material in memory.
    for asset_path, relative_file in MATERIALS:
        source_file = os.path.join(project_content, relative_file.replace("/", os.sep))
        if not os.path.isfile(source_file):
            raise RuntimeError("Source package is missing: {}".format(source_file))
        backup_file = os.path.join(backup_dir, os.path.basename(source_file))
        original_hash = backup_package(source_file, backup_file)
        material = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(material, unreal.Material):
            raise RuntimeError("Expected a base Material at {}".format(asset_path))
        packages.append({
            "asset_path": asset_path,
            "source_file": source_file,
            "backup_file": backup_file,
            "original_hash": original_hash,
            "material": material,
            "usage_before": bool(library.has_material_usage(material, usage)),
        })

    # Require all usage permutations to compile before saving any package.
    for package in packages:
        material = package["material"]
        if not package["usage_before"]:
            library.set_base_material_usage(material, usage, True)
        if not library.has_material_usage(material, usage):
            raise RuntimeError("Usage flag did not enable on {}".format(package["asset_path"]))
        errors = list(library.recompile_material(material))
        if errors:
            raise RuntimeError("{} failed material compilation: {}".format(package["asset_path"], errors))

    # Persist only after both materials have passed compilation.
    for package in packages:
        if not unreal.EditorAssetLibrary.save_loaded_asset(package["material"]):
            raise RuntimeError("Could not save {}".format(package["asset_path"]))

    reports = []
    for package in packages:
        material = unreal.EditorAssetLibrary.load_asset(package["asset_path"])
        enabled = bool(library.has_material_usage(material, usage))
        current_hash = sha256_file(package["source_file"])
        if not enabled:
            raise RuntimeError("Post-save usage check failed for {}".format(package["asset_path"]))
        if current_hash == package["original_hash"]:
            raise RuntimeError("Package hash did not change after saving {}".format(package["asset_path"]))
        reports.append({
            "path": package["asset_path"],
            "usage_before": package["usage_before"],
            "usage_after": enabled,
            "backup": package["backup_file"],
            "original_sha256": package["original_hash"],
            "saved_sha256": current_hash,
        })
    unreal.log("[StarterInstancedUsage] COMPLETE {}".format(reports))


try:
    main()
except Exception:
    unreal.log_error("[StarterInstancedUsage] FAILED " + traceback.format_exc())
    raise
