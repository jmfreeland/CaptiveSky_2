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
#>
param(
	[double]$Hour = -1,
	[string]$Only = "",
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor-Cmd.exe not found under $EngineDir; pass -EngineDir." }
$log = Join-Path $env:TEMP "CaptiveSky_Viewpoints.log"

$extra = @()
if ($Hour -ge 0) { $extra += "-ViewpointHour=$Hour" }
if ($Only) { $extra += "-ViewpointOnly=$Only" }

& $editor $project -ExecCmds="Automation RunTests CaptiveSky2.Visual.Viewpoints" -TestExit="Automation Test Queue Empty" `
	-unattended -RenderOffscreen -nosplash -nosound -NoZen "-abslog=$log" @extra | Out-Null

$lines = Select-String -Path $log -Pattern "Test Completed|Viewpoint captures saved|LogAutomationController: Error" | ForEach-Object { $_.Line }
$lines
if (-not ($lines -match "Result=\{Success\}")) { throw "Viewpoint capture failed; see $log" }
