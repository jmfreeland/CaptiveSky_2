<#
.SYNOPSIS
Opens the Island in spectator mode: a slow camera over the journey views that cuts to residents
when they speak or make something lasting.

.DESCRIPTION
Runs the project as a game (not the editor) with -Spectator. Residents think and remember as in any
play session, so the usual session limits apply (Config/DefaultGame.ini: at most 30 real minutes and
120 model requests), after which the game closes. Press ` and type Island.Spectate to toggle back to
normal control.

.EXAMPLE
./Scripts/Start-Spectator.ps1
./Scripts/Start-Spectator.ps1 -Windowed -Shots
#>
param(
	[switch]$Windowed,
	[switch]$Shots,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir." }

$arguments = @($project, "/Game/Maps/Island", "-game", "-Spectator")
if ($Windowed) { $arguments += @("-windowed", "-ResX=1600", "-ResY=900") } else { $arguments += "-fullscreen" }
if ($Shots) { $arguments += "-SpectatorShots" } # one frame per shot under Saved/Screenshots/Spectator
& $editor @arguments
