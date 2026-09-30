<#
.SYNOPSIS
Renders the fixed Island viewpoints in Config/IslandViewpoints.json to PNGs under Saved/Viewpoints/.

.DESCRIPTION
Starts a headless editor on the Island map with an offscreen RHI and runs the
CaptiveSky2.Visual.Viewpoints automation test. Captures come from the editor world only: no play
session starts, so no agents think, no gateway turns are claimed, and no world state is written.
The editor must be built first. Close any running editor on this project before running.

.EXAMPLE
./Scripts/Capture-Viewpoints.ps1
./Scripts/Capture-Viewpoints.ps1 -Hour 7.5 -Only Tideglass
./Scripts/Capture-Viewpoints.ps1 -Hour 8 -Only Tideglass -GroundCover
./Scripts/Capture-Viewpoints.ps1 -NightFireflies -Only Firefly -NoWorldState
./Scripts/Capture-Viewpoints.ps1 -NightFireflies -Only Firefly -NoWorldState -Day 1
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only WindArchOverlook -LandscapeWetness 0
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only WindArchOverlook -LandscapeWetness 1
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only WindArchOverlook -CompareLandscapeWetness
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only TideglassGroundDetail -CompareLandscapeWetness -LandscapePuddlePreview -LandscapeMaterial /Game/Materials/MI_Island_Landscape_WetPrototype
#>
param(
	[double]$Hour = -1,
	[int]$Day = 0,
	[double]$LandscapeWetness = -1,
	[switch]$CompareLandscapeWetness,
	[switch]$LandscapePuddlePreview,
	[string]$LandscapeMaterial = "",
	[string]$Only = "",
	[switch]$NightFireflies,
	[switch]$GroundCover,
	[switch]$NoWorldState,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor-Cmd.exe not found under $EngineDir; pass -EngineDir." }
$log = Join-Path $env:TEMP "CaptiveSky_Viewpoints.log"
$shaderWorkingDir = Join-Path (Split-Path $project) "Saved\ShaderWorking"
New-Item -ItemType Directory -Force -Path $shaderWorkingDir | Out-Null

$extra = @()
if ($NightFireflies -and $Hour -lt 0) { $Hour = 20 }
if ($Hour -ge 0) { $extra += "-ViewpointHour=$Hour" }
if ($Day -gt 0) { $extra += "-ViewpointDay=$Day" }
if ($LandscapeWetness -lt -1 -or $LandscapeWetness -gt 1) { throw "-LandscapeWetness must be -1 (not set) or between 0 and 1." }
if ($LandscapeWetness -ge 0) {
	if ($CompareLandscapeWetness) { throw "Use either -LandscapeWetness or -CompareLandscapeWetness, not both." }
	$extra += "-ViewpointLandscapeWetness=$LandscapeWetness"
}
if ($CompareLandscapeWetness) { $extra += "-ViewpointLandscapeWetnessPair" }
if ($LandscapePuddlePreview) {
	if (-not $CompareLandscapeWetness) { throw "-LandscapePuddlePreview requires -CompareLandscapeWetness." }
	if (-not $LandscapeMaterial) { throw "-LandscapePuddlePreview requires -LandscapeMaterial." }
	$extra += "-ViewpointLandscapePuddlePreview"
	$extra += "-ViewpointLandscapeMaterial=$LandscapeMaterial"
} elseif ($LandscapeMaterial) {
	throw "-LandscapeMaterial is only valid with -LandscapePuddlePreview."
}
if ($Only) { $extra += "-ViewpointOnly=$Only" }
if ($NightFireflies) { $extra += "-ViewpointNightFireflies" }
if ($GroundCover) { $extra += "-ViewpointGroundCover" }
if ($NoWorldState) { $extra += "-ViewpointNoWorldState" }

& $editor $project -ExecCmds="Automation RunTests CaptiveSky2.Visual.Viewpoints" -TestExit="Automation Test Queue Empty" `
	-unattended -RenderOffscreen -nosplash -nosound -NoZen -DDC-ForceMemoryCache "-shaderworkingdir=$shaderWorkingDir" "-abslog=$log" @extra | Out-Null

$lines = Select-String -Path $log -Pattern "Test Completed|Viewpoint captures saved|LogAutomationController: Error" | ForEach-Object { $_.Line }
$lines
if (-not ($lines -match "Result=\{Success\}")) { throw "Viewpoint capture failed; see $log" }
