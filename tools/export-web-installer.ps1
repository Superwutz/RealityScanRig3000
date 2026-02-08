param(
  [string]$EnvName = "esp32s3",
  [string]$OutRoot = "release/web-installer",
  [string]$Version = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$pio = "platformio"
if (-not (Test-Path $pio)) {
  throw "PlatformIO executable not found at $pio"
}

if ([string]::IsNullOrWhiteSpace($Version)) {
  $gitSha = (git -C $repoRoot rev-parse --short HEAD).Trim()
  if (-not $gitSha) { $gitSha = "dev" }
  $Version = "v-$gitSha"
}

& $pio run -e $EnvName

$buildDir = Join-Path $repoRoot ".pio\build\$EnvName"
$bootloader = Join-Path $buildDir "bootloader.bin"
$partitions = Join-Path $buildDir "partitions.bin"
$firmware = Join-Path $buildDir "firmware.bin"
$bootApp = "boot_app0.bin"

foreach ($f in @($bootloader, $partitions, $firmware, $bootApp)) {
  if (-not (Test-Path $f)) { throw "Missing required binary: $f" }
}

$dest = Join-Path $repoRoot (Join-Path $OutRoot $Version)
New-Item -ItemType Directory -Force -Path $dest | Out-Null

Copy-Item $bootloader (Join-Path $dest "bootloader.bin") -Force
Copy-Item $partitions (Join-Path $dest "partitions.bin") -Force
Copy-Item $bootApp (Join-Path $dest "boot_app0.bin") -Force
Copy-Item $firmware (Join-Path $dest "firmware.bin") -Force

$manifest = @"
{
  "name": "RealityScanRig3000",
  "version": "$Version",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "bootloader.bin", "offset": 0 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "firmware.bin", "offset": 65536 }
      ]
    }
  ]
}
"@

$manifestPath = Join-Path $dest "manifest.json"
[System.IO.File]::WriteAllText($manifestPath, $manifest, (New-Object System.Text.UTF8Encoding($false)))

Write-Output "Export complete:"
Write-Output "  $dest"
Write-Output "Upload this folder to static hosting and point esp-web-tools to manifest.json"
