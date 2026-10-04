<#
.SYNOPSIS
Restores original locally installed PlantFactory material functions into a content-only mount.
.DESCRIPTION
Copies vendor-owned uassets only, never executable code. Does not overwrite differing
files or edit existing plant meshes/materials. Assets remain out of Git; use only with
your own licensed installation. Run again after restoring the repo on another machine.
#>
param(
    [string]$SourceContent = 'C:\Program Files\e-on software\PlantFactory 2024\plugins\Unreal\Content',
    [string]$ProjectDir = (Split-Path $PSScriptRoot -Parent)
)
$ErrorActionPreference = 'Stop'
$resolvedProject = (Resolve-Path -LiteralPath $ProjectDir).Path
if (-not (Test-Path -LiteralPath (Join-Path $resolvedProject 'CaptiveSky_2.uproject'))) {
    throw 'ProjectDir must be a CaptiveSky_2 checkout or isolated scratch project.'
}
$resolvedSource = (Resolve-Path -LiteralPath $SourceContent).Path
foreach ($requiredAsset in @('StaticBillboard.uasset', 'BillboardMappedNormals.uasset')) {
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedSource $requiredAsset))) {
        throw "Required original function missing: $requiredAsset"
    }
}
$pluginDir = Join-Path $resolvedProject 'Plugins\PlantFactoryPlugin'
$destination = Join-Path $pluginDir 'Content'
$descriptorSource = Join-Path (Split-Path $PSScriptRoot -Parent) 'Plugins\PlantFactoryPlugin\PlantFactoryPlugin.uplugin'
if (-not (Test-Path -LiteralPath $descriptorSource)) { throw 'Tracked content-only descriptor is missing.' }
$assets = @(Get-ChildItem -LiteralPath $resolvedSource -Filter '*.uasset' -File)
# Preflight every existing target before copying any asset. A different version requires explicit review.
foreach ($asset in $assets) {
    $target = Join-Path $destination $asset.Name
    if ((Test-Path -LiteralPath $target) -and
        (Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath $asset.FullName).Hash) {
        throw "Existing asset differs; refusing to overwrite: $target"
    }
}
New-Item -ItemType Directory -Force -Path $destination | Out-Null
$descriptorTarget = Join-Path $pluginDir 'PlantFactoryPlugin.uplugin'
if (-not (Test-Path -LiteralPath $descriptorTarget)) {
    Copy-Item -LiteralPath $descriptorSource -Destination $descriptorTarget
} elseif ((Get-FileHash -LiteralPath $descriptorTarget).Hash -ne (Get-FileHash -LiteralPath $descriptorSource).Hash) {
    throw "Existing descriptor differs; refusing to overwrite: $descriptorTarget"
}
foreach ($asset in $assets) {
    Copy-Item -LiteralPath $asset.FullName -Destination (Join-Path $destination $asset.Name)
}
"Restored $($assets.Count) original function assets into $destination"
