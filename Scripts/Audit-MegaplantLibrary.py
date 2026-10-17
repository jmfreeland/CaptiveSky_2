"""Read-only inventory of imported Megaplant canopy and PVE assets.

Run from the Unreal Editor Python console with:
    py "D:/Projects - Athena/Unreal/CaptiveSky_2/Scripts/Audit-MegaplantLibrary.py"

The report is written only to the Output Log. It does not load, alter, or save
project assets or the Island map.
"""

try:
    import unreal
except ImportError as exc:
    raise RuntimeError("Run this script from Unreal Editor with the Python Editor Script Plugin enabled.") from exc


ROOT = "/Game/Megaplant_Library"


def _field_text(value, field):
    try:
        return str(getattr(value, field))
    except Exception:
        return ""


def _asset_row(asset_data):
    package = _field_text(asset_data, "package_name")
    name = _field_text(asset_data, "asset_name")
    class_path = _field_text(asset_data, "asset_class_path")
    if not class_path:
        class_path = _field_text(asset_data, "asset_class")
    return package, name, class_path


def _is_canopy_candidate(name):
    if name.startswith("PVE_"):
        return True
    return name.startswith("Tree_") and name.rsplit("_", 1)[-1] in {"A", "B", "C", "D"}


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(ROOT, recursive=True)
    rows = sorted(
        (row for row in (_asset_row(asset) for asset in assets) if _is_canopy_candidate(row[1])),
        key=lambda row: (row[2], row[0]),
    )

    unreal.log_warning(
        "Megaplant audit (read-only): %d packages under %s; %d canopy/PVE candidates."
        % (len(assets), ROOT, len(rows))
    )
    for package, name, class_path in rows:
        unreal.log("MEGAPLANT_ASSET class=%s package=%s name=%s" % (class_path, package, name))
    if not assets:
        unreal.log_warning("Megaplant audit found no registered assets. Check that the library is mounted and scanned.")


main()
