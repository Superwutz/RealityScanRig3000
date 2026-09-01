param(
  [string]$Url = "",
  [switch]$ServeDocs,
  [switch]$DeviceOnly,
  [switch]$Headed
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

if (-not (Test-Path (Join-Path $repoRoot "package.json"))) {
  throw "package.json missing"
}

cmd /c npm install
cmd /c npx playwright install chromium

$serverProc = $null
$webDir = Join-Path $repoRoot "web"
try {
  if ($ServeDocs) {
    if (-not (Test-Path (Join-Path $webDir "index.html"))) {
      throw "web/index.html missing (UI sources live in web/)"
    }
    $serverProc = Start-Process -FilePath "python" -ArgumentList "-m", "http.server", "8080" -WorkingDirectory $webDir -PassThru
    Start-Sleep -Seconds 1
    $Url = "http://127.0.0.1:8080"
  }

  if (-not $Url) {
    $Url = "http://127.0.0.1:8080"
  }

  $env:UI_URL = $Url
  $args = @("playwright", "test")
  if ($Headed) { $args += "--headed" }
  if ($DeviceOnly) { $args += @("-g", "@device") }
  cmd /c npx @args
}
finally {
  if ($serverProc -and -not $serverProc.HasExited) {
    Stop-Process -Id $serverProc.Id -Force
  }
}
