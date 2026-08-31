param(
  [Parameter(Mandatory = $true)]
  [ValidatePattern("^v\d+\.\d+\.\d+$")]
  [string]$Version,
  [string]$EnvName = "esp32s3",
  [string]$StageRoot = ".staging/web-installer",
  [string]$UiDeviceUrl = "http://scanrig.local",
  [switch]$SkipUiTests,
  [switch]$SkipDeviceTests,
  [switch]$Commit,
  [switch]$Tag,
  [switch]$Push
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$stageScript = Join-Path $PSScriptRoot "stage-web-installer.ps1"
$uiTestScript = Join-Path $PSScriptRoot "ui-test.ps1"
$mainCpp = Join-Path $repoRoot "src/main.cpp"
$docsDir = Join-Path $repoRoot "docs"

function Get-GitText {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$Args
  )
  $lines = & git -C $repoRoot @Args
  if ($null -eq $lines) { return "" }
  if ($lines -is [System.Array]) {
    return (($lines -join "`n").Trim())
  }
  return ([string]$lines).Trim()
}

if (-not (Test-Path $stageScript)) {
  throw "Missing script: $stageScript"
}
if (-not (Test-Path $uiTestScript)) {
  throw "Missing script: $uiTestScript"
}
if (-not (Test-Path $mainCpp)) {
  throw "Missing file: $mainCpp"
}
if (-not (Test-Path $docsDir)) {
  throw "Missing docs directory: $docsDir"
}
if ($Push -and (-not $Commit -or -not $Tag)) {
  throw "-Push requires -Commit and -Tag."
}

$branch = Get-GitText @("rev-parse", "--abbrev-ref", "HEAD")
if ($branch -ne "main") {
  throw "Release must run from 'main'. Current branch: $branch"
}

$worktreeState = Get-GitText @("status", "--porcelain")
if ($worktreeState) {
  throw "Working tree must be clean before release."
}

$runUiTests = -not $SkipUiTests
if ($runUiTests) {
  Write-Output "Running UI smoke tests (local rendered UI)..."
  powershell -ExecutionPolicy Bypass -File $uiTestScript -ServeDocs
  if ($LASTEXITCODE -ne 0) {
    throw "UI smoke tests failed."
  }

  if (-not $SkipDeviceTests) {
    Write-Output "Running UI device tests against $UiDeviceUrl ..."
    powershell -ExecutionPolicy Bypass -File $uiTestScript -Url $UiDeviceUrl -DeviceOnly
    if ($LASTEXITCODE -ne 0) {
      throw "UI device tests failed."
    }
  } else {
    Write-Output "Skipping UI device tests (-SkipDeviceTests)."
  }
} else {
  Write-Output "Skipping all UI tests (-SkipUiTests)."
}

$manifestPath = Join-Path $docsDir "manifest.json"
$manifestBefore = Get-Content -Raw $manifestPath | ConvertFrom-Json
$currentVersion = [version]($manifestBefore.version.TrimStart("v"))
$nextVersion = [version]($Version.TrimStart("v"))
if ($nextVersion -le $currentVersion) {
  throw "Version must be greater than docs/manifest.json version ($($manifestBefore.version))."
}

$plainVersion = $Version.TrimStart("v")
$mainCppRaw = Get-Content -Raw $mainCpp
$mainCppRaw = [regex]::Replace(
  $mainCppRaw,
  '(#define\s+SCANRIG_FW_VERSION\s+")([^"]+)(")',
  "`${1}$plainVersion`${3}",
  1
)
$mainCppRaw = [regex]::Replace(
  $mainCppRaw,
  '(#define\s+SCANRIG_UI_VERSION\s+")([^"]+)(")',
  "`${1}$plainVersion`${3}",
  1
)
[System.IO.File]::WriteAllText($mainCpp, $mainCppRaw, (New-Object System.Text.UTF8Encoding($false)))

# Keep the UI cache-buster in sync and regenerate the embedded assets header.
$webIndexPath = Join-Path $repoRoot "web/index.html"
$webIndexRaw = Get-Content -Raw $webIndexPath
$webIndexRaw = [regex]::Replace(
  $webIndexRaw,
  '(\./app\.js\?v=)([^"]+)(")',
  "`${1}$plainVersion`${3}",
  1
)
[System.IO.File]::WriteAllText($webIndexPath, $webIndexRaw, (New-Object System.Text.UTF8Encoding($false)))
python (Join-Path $repoRoot "tools/webassets.py") embed
if ($LASTEXITCODE -ne 0) { throw "tools/webassets.py embed failed" }

powershell -ExecutionPolicy Bypass -File $stageScript -EnvName $EnvName -Version $Version -StageRoot $StageRoot

$stageDir = Join-Path $repoRoot (Join-Path $StageRoot $Version)
$releaseFiles = @(
  "bootloader.bin",
  "partitions.bin",
  "boot_app0.bin",
  "firmware.bin",
  "manifest.json",
  "index.html"
)
foreach ($name in $releaseFiles) {
  $src = Join-Path $stageDir $name
  $dst = Join-Path $docsDir $name
  if (-not (Test-Path $src)) {
    throw "Missing generated release file: $src"
  }
  Copy-Item $src $dst -Force
}

$manifestAfter = Get-Content -Raw $manifestPath | ConvertFrom-Json
if ($manifestAfter.version -ne $Version) {
  throw "Manifest version mismatch after export. Expected $Version, found $($manifestAfter.version)."
}

Write-Output "Release files prepared for $Version."
Write-Output "Updated:"
foreach ($name in @("src/main.cpp", "docs/manifest.json", "docs/index.html", "docs/bootloader.bin", "docs/partitions.bin", "docs/boot_app0.bin", "docs/firmware.bin")) {
  Write-Output "  $name"
}

if ($Commit) {
  git -C $repoRoot add -- src/main.cpp docs/manifest.json docs/index.html docs/bootloader.bin docs/partitions.bin docs/boot_app0.bin docs/firmware.bin
  git -C $repoRoot commit -m "release: $Version"
  Write-Output "Created commit: release: $Version"
}

if ($Tag) {
  $existingTag = Get-GitText @("tag", "-l", $Version)
  if ($existingTag) {
    throw "Tag already exists: $Version"
  }
  git -C $repoRoot tag -a $Version -m "Release $Version"
  Write-Output "Created tag: $Version"
}

if ($Push) {
  git -C $repoRoot push origin main
  git -C $repoRoot push origin $Version
  Write-Output "Pushed main and tag $Version to origin."
}
