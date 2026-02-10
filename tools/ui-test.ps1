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
$tmpUiDir = Join-Path $repoRoot ".tmp_ui"
try {
  if ($ServeDocs) {
    if (Test-Path $tmpUiDir) { Remove-Item -Path $tmpUiDir -Recurse -Force }
    New-Item -ItemType Directory -Path $tmpUiDir | Out-Null

    @'
import re
from pathlib import Path

h = Path("src/web_assets.h").read_text(encoding="utf-8")
for name, out in [("INDEX_HTML", "index.html"), ("STYLE_CSS", "style.css"), ("APP_JS", "app.js")]:
    m = re.search(rf"static const uint8_t {name}\[\] PROGMEM = \{{(.*?)\}};\s*\n\s*static const size_t", h, re.S)
    if not m:
        raise SystemExit(f"missing {name}")
    vals = re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))
    Path(".tmp_ui", out).write_bytes(bytes(int(v, 16) for v in vals))
'@ | python -

    $serverProc = Start-Process -FilePath "python" -ArgumentList "-m", "http.server", "8080" -WorkingDirectory $tmpUiDir -PassThru
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
  if (Test-Path $tmpUiDir) {
    Remove-Item -Path $tmpUiDir -Recurse -Force
  }
}
