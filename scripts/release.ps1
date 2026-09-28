#Requires -Version 5.1
# Fully unattended. `pixi run release <major|minor|patch|nightly|X.Y.Z>` is the
# authorization - there is no confirmation gate. Preconditions (on main, clean
# tree, tag absent, valid semver) are the safety net; any failure exits
# non-zero with a one-line diagnostic.
param(
    [string]$Version,
    # Ship a release even when there are no user-facing commits since the last
    # tag (writes a maintenance changelog entry instead of aborting).
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# THIRD-PARTY-NOTICES.md names the cameraunlock-core commit compiled into the
# release ZIPs, and bumping the submodule does not touch it. Packaging refuses
# to ship that mismatch, so a bump with no notices edit stopped the release
# here, or in CI once the tag had already been pushed. Re-sync it and let this
# release carry the correction.
$noticesRoot = Split-Path -Parent $PSScriptRoot
& git -C $noticesRoot diff --quiet -- THIRD-PARTY-NOTICES.md
if ($LASTEXITCODE -ne 0) { throw "THIRD-PARTY-NOTICES.md has uncommitted edits. Commit or discard them, then re-run." }
& (Join-Path $noticesRoot 'cameraunlock-core\scripts\sync-core-notices.ps1') -Repo $noticesRoot
if ($LASTEXITCODE -ne 0) { throw "sync-core-notices.ps1 exited $LASTEXITCODE - fix THIRD-PARTY-NOTICES.md before releasing." }
& git -C $noticesRoot diff --quiet -- THIRD-PARTY-NOTICES.md
if ($LASTEXITCODE -ne 0) {
    & git -C $noticesRoot commit -q -m 'chore: record the cameraunlock-core commit this build compiles' -- THIRD-PARTY-NOTICES.md
    if ($LASTEXITCODE -ne 0) { throw "Could not commit the re-synced THIRD-PARTY-NOTICES.md." }
    Write-Host 'THIRD-PARTY-NOTICES.md re-synced to the pinned cameraunlock-core commit.' -ForegroundColor Yellow
}
$projectDir = Split-Path -Parent $PSScriptRoot
Import-Module (Join-Path $projectDir "cameraunlock-core/powershell/ReleaseWorkflow.psm1") -Force

if ([string]::IsNullOrWhiteSpace($Version)) {
    Write-Error "Usage: pixi run release <major|minor|patch|nightly|X.Y.Z>"
    exit 1
}

if ($Version -eq 'nightly') {
    & (Join-Path $PSScriptRoot 'release-nightly.ps1')
    exit $LASTEXITCODE
}

$pixiPath       = Join-Path $projectDir "pixi.toml"
$changelogPath  = Join-Path $projectDir "CHANGELOG.md"
$installCmdPath = Join-Path $projectDir "scripts/install.cmd"
$cmakePath      = Join-Path $projectDir "CMakeLists.txt"
$manifestPath   = Join-Path $projectDir "launcher-manifest.json"

# 1. Resolve + validate version against the canonical source (pixi.toml).
$pixiContent = Get-Content $pixiPath -Raw
if ($pixiContent -notmatch '(?m)^version\s*=\s*"([^"]+)"') {
    throw "No version field found in $pixiPath"
}
$currentVersion = $matches[1]
$newVersion = Resolve-ReleaseVersion -Argument $Version -CurrentVersion $currentVersion
if (-not (Test-SemanticVersion -Version $newVersion)) {
    throw "Resolved version '$newVersion' is not valid semver (X.Y.Z)."
}
Write-Host "Releasing v$newVersion (current v$currentVersion)" -ForegroundColor Cyan
Assert-ReleaseNotBelowCanonicalSince -RepoRoot $projectDir -Version $newVersion

# 2. Preconditions - fail fast, never prompt.
$branch = (git rev-parse --abbrev-ref HEAD).Trim()
if ($branch -ne "main") { throw "Releases must run on 'main' (currently on '$branch')." }
if (-not (Test-CleanGitStatus)) { throw "Working tree is not clean. Commit or stash first." }
if (Test-GitTagExists -Tag "v$newVersion") { throw "Tag v$newVersion already exists." }

Write-Host "Running the full test suite..." -ForegroundColor Cyan
Push-Location $projectDir
try {
    pixi run test
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: pixi run test failed. Nothing was changed." -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
}

# 3. Generate the changelog from commits since the last tag. This is the gate
#    that aborts when there are no user-facing commits, so run it BEFORE
#    mutating any version files or building - a failure here then leaves a
#    clean tree instead of stranding a half-applied version bump with no tag.
Write-Host "Generating CHANGELOG..." -ForegroundColor Cyan
try {
    New-ChangelogFromCommits -ChangelogPath $changelogPath -Version $newVersion `
        -ArtifactPaths @('src/', 'cameraunlock-core/', 'scripts/install.cmd', 'scripts/uninstall.cmd') `
        -Maintenance:$Force | Out-Null
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    if (-not $Force) {
        Write-Host "No user-facing changes to release. Re-run with -Force for a maintenance release." -ForegroundColor Yellow
    }
    exit 1
}

# 4. Bump the canonical version, then mirror it into every derived copy.
$pixiContent = $pixiContent -replace '(?m)^(version\s*=\s*")[^"]+(")', "`${1}$newVersion`${2}"
$pixiContent | Set-Content $pixiPath -NoNewline

$cmakeContent = Get-Content $cmakePath -Raw
if ($cmakeContent -notmatch 'project\(AssettoCorsaEvoHeadTracking VERSION [0-9]+\.[0-9]+\.[0-9]+') {
    throw "No project VERSION found in $cmakePath"
}
$cmakeContent = $cmakeContent -replace '(project\(AssettoCorsaEvoHeadTracking VERSION )[0-9]+\.[0-9]+\.[0-9]+', "`${1}$newVersion"
$cmakeContent | Set-Content $cmakePath -NoNewline

# -Raw + a digits-only regex keeps install.cmd's CRLF endings intact (a .cmd
# silently fails on Windows if rewritten LF).
$installContent = Get-Content $installCmdPath -Raw
if ($installContent -notmatch '(?m)^set "MOD_VERSION=[0-9]+\.[0-9]+\.[0-9]+"') {
    throw "No MOD_VERSION line found in $installCmdPath"
}
$installContent = $installContent -replace '(?m)^(set "MOD_VERSION=)[0-9]+\.[0-9]+\.[0-9]+(")', "`${1}$newVersion`${2}"
$installContent | Set-Content $installCmdPath -NoNewline

$manifestContent = Get-Content $manifestPath -Raw
if ($manifestContent -notmatch '(?m)^\s*"version"\s*:\s*"[0-9]+\.[0-9]+\.[0-9]+"') {
    throw "No version field found in $manifestPath"
}
$manifestContent = $manifestContent -replace '(?m)^(\s*"version"\s*:\s*")[0-9]+\.[0-9]+\.[0-9]+(")', "`${1}$newVersion`${2}"
$manifestContent | Set-Content $manifestPath -NoNewline

# 5. Release-config build.
Write-Host "Building release..." -ForegroundColor Cyan
pixi run build
if ($LASTEXITCODE -ne 0) { throw "Build failed; aborting release." }

# 6. Commit the version bump + changelog. "Release v..." matches the build.yml
#    skip guard so CI doesn't double-build this commit.
git add $pixiPath $changelogPath $installCmdPath $manifestPath $cmakePath
if ($LASTEXITCODE -ne 0) { throw "git add failed." }
git commit -m "Release v$newVersion"
if ($LASTEXITCODE -ne 0) { throw "git commit failed." }

# 7 + 8. Annotated tag, then push commits + tag (triggers release.yml).
New-ReleaseTag -Version $newVersion -Message "Release v$newVersion" -Branch "main"

Write-Host "Released v$newVersion. CI release workflow will publish the ZIPs." -ForegroundColor Green
