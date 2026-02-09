param(
  [string]$EnvName = "esp32s3",
  [string]$OutRoot = "release/web-installer",
  [string]$Version = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-PlatformIOExe {
  $cmd = Get-Command platformio -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }

  $cmd = Get-Command pio -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }

  $candidates = @(
    (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"),
    (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe")
  )

  foreach ($candidate in $candidates) {
    if ($candidate -and (Test-Path $candidate)) { return $candidate }
  }

  throw "PlatformIO executable not found. Ensure 'platformio' or 'pio' is available in PATH."
}

function Resolve-BootApp0Path {
  $coreRoots = @()
  if ($env:PLATFORMIO_CORE_DIR) { $coreRoots += $env:PLATFORMIO_CORE_DIR }
  if ($env:USERPROFILE) { $coreRoots += (Join-Path $env:USERPROFILE ".platformio") }

  foreach ($core in $coreRoots) {
    if (-not (Test-Path $core)) { continue }
    $pkg = Join-Path $core "packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
    if (Test-Path $pkg) { return $pkg }

    $match = Get-ChildItem -Path (Join-Path $core "packages") -Recurse -Filter "boot_app0.bin" -ErrorAction SilentlyContinue |
      Where-Object { $_.FullName -like "*framework-arduinoespressif32*" } |
      Select-Object -First 1
    if ($match) { return $match.FullName }
  }

  throw "boot_app0.bin not found in PlatformIO packages. Build once with PlatformIO and retry."
}

$pio = Resolve-PlatformIOExe

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
$bootApp = Resolve-BootApp0Path

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
