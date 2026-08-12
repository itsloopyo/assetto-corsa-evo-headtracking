#!/usr/bin/env pwsh
#Requires -Version 5.1
# Dev loop: copy the built .asi and the vendored ASI loader into the game folder.

[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Config = 'Release',
    [string]$GamePath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Split-Path -Parent $scriptDir

Import-Module (Join-Path $projectDir 'cameraunlock-core/powershell/GamePathDetection.psm1') -Force

if (-not $GamePath) {
    $GamePath = Find-GamePath -GameId 'assetto-corsa-evo'
}
if (-not $GamePath -or -not (Test-Path $GamePath)) {
    throw "Assetto Corsa EVO not found. Pass -GamePath explicitly."
}

$asi = Join-Path $projectDir "build/$Config/AssettoCorsaEvoHeadTracking.asi"
if (-not (Test-Path $asi)) { throw "Build output not found: $asi. Run 'pixi run build' first." }

$loader = Join-Path $projectDir 'vendor/ultimate-asi-loader/dinput8.dll'
if (-not (Test-Path $loader)) { throw "Vendored ASI loader missing. Run 'pixi run update-deps'." }

Copy-Item $asi (Join-Path $GamePath 'AssettoCorsaEvoHeadTracking.asi') -Force
Write-Host "  deployed AssettoCorsaEvoHeadTracking.asi" -ForegroundColor DarkGray

$loaderTarget = Join-Path $GamePath 'dinput8.dll'
if (-not (Test-Path $loaderTarget)) {
    Copy-Item $loader $loaderTarget -Force
    Write-Host "  deployed dinput8.dll (Ultimate ASI Loader)" -ForegroundColor DarkGray
} else {
    Write-Host "  dinput8.dll already present, left alone" -ForegroundColor DarkGray
}

Write-Host "Deployed to $GamePath" -ForegroundColor Green
