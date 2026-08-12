#!/usr/bin/env pwsh
#Requires -Version 5.1
# Build and run the unit tests in their own build directory so the normal
# build/ tree never carries a test binary.

[CmdletBinding()]
param([ValidateSet('Release', 'Debug')][string]$Config = 'Debug')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $PSScriptRoot
$buildDir   = Join-Path $projectDir 'build-tests'

cmake -S $projectDir -B $buildDir -A x64 -DACE_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed ($LASTEXITCODE)" }

cmake --build $buildDir --config $Config --target ace_tests
if ($LASTEXITCODE -ne 0) { throw "Test build failed ($LASTEXITCODE)" }

ctest --test-dir $buildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed ($LASTEXITCODE)" }

Write-Host 'All tests passed' -ForegroundColor Green
