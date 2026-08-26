#!/usr/bin/env pwsh
#Requires -Version 5.1
# Read the PE fingerprint (TimeDateStamp / SizeOfImage / CheckSum) from an
# AssettoCorsaEVO.exe on disk and compare it against every build profile in
# src/builds/. First thing to run when a user reports the dormant
# "unknown build" log line, and the first step of a post-patch rederive.
#
# This answers "does this EXE have a profile". The second step answers whether
# a new profile can reuse the pinned numbers:
#
#     pixi run verify-camera-profile
#
# which walks RTTI in the EXE and checks the compute slot and transform offset
# still hold. Run it before pasting the template below - the numbers it prints
# are the current profile's, not measurements of this EXE.

[CmdletBinding()]
param([string]$ExePath)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $PSScriptRoot

if (-not $ExePath) {
    Import-Module (Join-Path $projectDir 'cameraunlock-core/powershell/GamePathDetection.psm1') -Force
    $gamePath = Find-GamePath -GameId 'assetto-corsa-evo'
    if (-not $gamePath) { throw "Assetto Corsa EVO not found. Pass -ExePath explicitly." }
    $ExePath = Join-Path $gamePath 'AssettoCorsaEVO.exe'
}
if (-not (Test-Path $ExePath)) { throw "EXE not found: $ExePath" }

$bytes = [System.IO.File]::ReadAllBytes($ExePath)
$peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
if ([BitConverter]::ToUInt32($bytes, $peOffset) -ne 0x00004550) { throw "Not a PE image: $ExePath" }

$coff    = $peOffset + 4
$optHdr  = $coff + 20
$timeDateStamp = [BitConverter]::ToUInt32($bytes, $coff + 4)
$sizeOfImage   = [BitConverter]::ToUInt32($bytes, $optHdr + 56)
$checkSum      = [BitConverter]::ToUInt32($bytes, $optHdr + 64)

$built = [DateTimeOffset]::FromUnixTimeSeconds($timeDateStamp).UtcDateTime
Write-Host "EXE: $ExePath"
Write-Host ("  TimeDateStamp 0x{0:X8}  ({1:yyyy-MM-dd HH:mm:ss} UTC)" -f $timeDateStamp, $built)
Write-Host ("  SizeOfImage   0x{0:X8}" -f $sizeOfImage)
Write-Host ("  CheckSum      0x{0:X8}" -f $checkSum)
Write-Host ""

$offsetsFile = Join-Path $projectDir 'src/builds/steam_offsets.cpp'
$known = Select-String -Path $offsetsFile -Pattern '\{\s*0x([0-9A-Fa-f]{8}),\s*0x([0-9A-Fa-f]{8}),\s*0x([0-9A-Fa-f]{8})\s*\}'
$matched = $false
foreach ($k in $known) {
    $t = [Convert]::ToUInt32($k.Matches[0].Groups[1].Value, 16)
    $s = [Convert]::ToUInt32($k.Matches[0].Groups[2].Value, 16)
    $c = [Convert]::ToUInt32($k.Matches[0].Groups[3].Value, 16)
    if ($t -eq $timeDateStamp -and $s -eq $sizeOfImage -and $c -eq $checkSum) {
        Write-Host "MATCH: this build already has a profile (steam_offsets.cpp line $($k.LineNumber))." -ForegroundColor Green
        $matched = $true
    }
}

if (-not $matched) {
    Write-Host "No profile matches this EXE. Append a new profile to src/builds/steam_offsets.cpp:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host ("extern const BuildProfile kSteamProfile_{0:yyyyMMdd} = {{" -f $built)
    Write-Host ("    `"steam-win64-{0:yyyyMMdd}`"," -f $built)
    Write-Host ("    {{ 0x{0:X8}, 0x{1:X8}, 0x{2:X8} }}," -f $timeDateStamp, $sizeOfImage, $checkSum)
    Write-Host "    {"
    Write-Host "        /* camera_compute_slot        */ 2,"
    Write-Host "        /* camera_out_transform       */ 0x2C,"
    Write-Host "        /* camera_out_transform_floats*/ 16,"
    Write-Host "    },"
    Write-Host "};"
    Write-Host ""
    Write-Host "Then add it to the TOP of kKnownProfiles in src/builds/build_registry.cpp."
    Write-Host "Verify the slot/offset still hold before shipping - leave them 0 to keep the"
    Write-Host "profile dormant until the rederive is done."
}
