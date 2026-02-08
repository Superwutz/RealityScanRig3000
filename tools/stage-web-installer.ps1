param(
  [string]$EnvName = "esp32s3",
  [string]$Version = "",
  [string]$StageRoot = ".staging/web-installer"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$exportScript = Join-Path $PSScriptRoot "export-web-installer.ps1"
if (-not (Test-Path $exportScript)) {
  throw "Missing export script: $exportScript"
}

if ([string]::IsNullOrWhiteSpace($Version)) {
  $gitSha = (git -C $repoRoot rev-parse --short HEAD).Trim()
  if (-not $gitSha) { $gitSha = "dev" }
  $Version = "v-$gitSha"
}

powershell -ExecutionPolicy Bypass -File $exportScript -EnvName $EnvName -OutRoot $StageRoot -Version $Version

$stageDir = Join-Path $repoRoot (Join-Path $StageRoot $Version)
$installerTemplate = Join-Path $repoRoot "deploy/web-installer/index.html"
if (-not (Test-Path $installerTemplate)) {
  throw "Missing installer template: $installerTemplate"
}

Copy-Item $installerTemplate (Join-Path $stageDir "index.html") -Force

Write-Output ""
Write-Output "Local staging bundle ready:"
Write-Output "  $stageDir"
Write-Output ""
Write-Output "Quick local test:"
Write-Output "  cd `"$stageDir`""
Write-Output "  python -m http.server 8000"
Write-Output "  open http://localhost:8000"
