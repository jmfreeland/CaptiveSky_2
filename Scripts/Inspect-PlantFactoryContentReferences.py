"""Read-only project Asset Registry audit for the local PlantFactory content mount.

Run with UE 5.8.3 UnrealEditor-Cmd, -NullRHI and -ExecutePythonScript.
This scans the registry and logs mounted plugin assets plus direct referencers;
it does not save or modify assets or levels.
"""

import json
import traceback

import unreal


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    registry.scan_paths_synchronous(["/Game", "/PlantFactoryPlugin"], False)

    options = unreal.AssetRegistryDependencyOptions()
    for field in (
        "include_hard_package_references",
        "include_soft_package_references",
        "include_searchable_names",
        "include_soft_management_references",
        "include_hard_management_references",
    ):
        try:
            options.set_editor_property(field, True)
        except Exception:
            pass

    plugin_assets = registry.get_assets_by_path("/PlantFactoryPlugin", recursive=True)
    report = {"plugin_asset_count": len(plugin_assets), "assets": [], "errors": []}
    for asset_data in sorted(plugin_assets, key=lambda item: str(item.package_name)):
        package = str(asset_data.package_name)
        entry = {
            "package": package,
            "asset_name": str(asset_data.asset_name),
            "class": str(asset_data.asset_class_path),
            "referencers": [],
            "error": None,
        }
        try:
            try:
                referencers = registry.get_referencers(asset_data.package_name, options)
            except TypeError:
                referencers = registry.get_referencers(asset_data.package_name)
            entry["referencers"] = sorted(str(item) for item in referencers)
        except Exception as error:
            entry["error"] = str(error)
            report["errors"].append({"package": package, "error": str(error)})
        report["assets"].append(entry)

    report["referencing_packages"] = sorted({
        referencer
        for entry in report["assets"]
        for referencer in entry["referencers"]
    })
    report["referencing_game_packages"] = [
        path for path in report["referencing_packages"] if path.startswith("/Game/")
    ]
    unreal.log("[PlantFactoryRegistryAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[PlantFactoryRegistryAudit] COMPLETE assets={} game_referencers={} errors={}".format(
        report["plugin_asset_count"],
        len(report["referencing_game_packages"]),
        len(report["errors"]),
    ))


try:
    main()
except Exception:
    unreal.log_error("[PlantFactoryRegistryAudit] FAILED " + traceback.format_exc())
    raise
