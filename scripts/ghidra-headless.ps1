#!/usr/bin/env pwsh
#Requires -Version 5.1
# Run a Jython/Python Ghidra script from scripts/ghidra/ against the Assetto
# Corsa EVO shipping EXE, headless.
#
# Usage: pixi run ghidra-script <script-name> [-ProgramExe <path>]
#        script-name is relative to scripts/ghidra/, with or without .py
#
# On first run the EXE is imported into a Ghidra project at C:\temp\ACEvo and
# auto-analyzed. That is slow: .text is ~48 MB. Subsequent runs reuse it.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [string]$ProgramExe,
    [string]$ProjectName = 'ACEvo',
    # analyzeHeadless.bat hardcodes MAXMEM=2G, which is far too small for a
    # binary this size, so the JVM is launched through launch.bat directly.
    [string]$MaxMem = '24G'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$GhidraRoot = 'C:\ProgramData\chocolatey\lib\ghidra\tools\ghidra_12.0_PUBLIC'
if (-not (Test-Path $GhidraRoot)) { throw "Ghidra not found at $GhidraRoot" }

$projectDir = Split-Path -Parent $PSScriptRoot

if (-not $ProgramExe) {
    Import-Module (Join-Path $projectDir 'cameraunlock-core/powershell/GamePathDetection.psm1') -Force
    $gamePath = Find-GamePath -GameId 'assetto-corsa-evo'
    if (-not $gamePath) { throw "Assetto Corsa EVO not found. Pass -ProgramExe explicitly." }
    $ProgramExe = Join-Path $gamePath 'AssettoCorsaEVO.exe'
}
if (-not (Test-Path $ProgramExe)) { throw "Game EXE not found at $ProgramExe" }

if (-not $Script.EndsWith('.py')) { $Script = "$Script.py" }
$scriptDir  = Join-Path $projectDir 'scripts/ghidra'
$scriptPath = Join-Path $scriptDir $Script
if (-not (Test-Path $scriptPath)) { throw "Script not found: $scriptPath" }

$projectRoot = "C:\temp\$ProjectName"
if (-not (Test-Path $projectRoot)) { New-Item -ItemType Directory -Path $projectRoot -Force | Out-Null }

$needImport = -not (Test-Path (Join-Path $projectRoot "$ProjectName.gpr"))
if ($needImport) {
    Write-Host "First-time import of $ProgramExe (slow: ~48 MB of code)" -ForegroundColor Cyan
}

# Ghidra 12 dropped bundled Jython, so .py scripts run through PyGhidra rather
# than analyzeHeadless -postScript (which fails with "Ghidra was not started
# with PyGhidra"). MaxMem is left to PyGhidra's own JVM defaults.
$env:GHIDRA_INSTALL_DIR = $GhidraRoot
$programName = [IO.Path]::GetFileName($ProgramExe)
$flag = if ($needImport) { 'True' } else { 'False' }

$py = @"
import pyghidra
pyghidra.run_script(
    binary_path=r'$ProgramExe' if $flag else None,
    script_path=r'$scriptPath',
    project_location=r'$projectRoot',
    project_name=r'$ProjectName',
    program_name=r'$programName',
    analyze=$flag,
    nested_project_location=False,
)
"@

& py -3 -c $py
exit $LASTEXITCODE
