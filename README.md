# RealityScanRig3000

RealityScanRig3000 is an ESP32-S3 firmware + web UI for automated dual-axis photogrammetry capture.

The project is designed for end users first:
- Install from a website over USB
- Run and configure from a browser UI
- Update firmware from the UI (OTA)

No IDE is required for normal installation and updates.

## Main Features

- Browser-based control UI hosted by the ESP32
- Dual-axis sequence engine (rotation + tilt)
- Preset-driven scan profiles
- Live status, progress, and error indicators
- Manual motion controls
- OTA update flow from the Settings screen
- Web installer for first-time flashing

## Supported Hardware

- ESP32-S3 board (`esp32-s3-devkitc-1` target)
- Revopoint Dual-axis Turntable

## Quick Start (End Users)

1. Open the installer page in desktop Chrome or Edge:  
   `https://superwutz.github.io/RealityScanRig3000/`
2. Connect your ESP32 board by USB.
3. Click `Install` and choose the serial device.
4. Wait for flash + reboot.
5. Open the rig UI at:
   - `http://scanrig.local`
   - or the IP shown by your router

## Firmware Updates (OTA)

After initial USB install, updates are done directly in the UI:

1. Open the rig UI.
2. Go to `Settings` -> `Firmware Update`.
3. Click `Check Updates`.
4. If a newer release is available, click `Update Now`.

The board reboots automatically after a successful update.

You can also upload a local firmware file via `Upload Firmware (.bin)`.

Default update manifest URL:

`https://superwutz.github.io/RealityScanRig3000/manifest.json`

## Network Configuration (No IDE)

Wi-Fi and static IP can be configured directly in the UI:

1. Open `Settings` -> `Network`
2. Enter SSID/password
3. Optionally enable static IPv4
4. Save (or Save + Reboot)

This is also the intended first-time setup path if your board cannot join your Wi-Fi yet.

AP fallback defaults:
- SSID: `RealityScanRig3000`
- Password: `ScanRigPW`

First-time Wi-Fi onboarding via AP fallback:
1. Connect your phone/laptop to `RealityScanRig3000` (`ScanRigPW`).
2. Open the rig UI (typically `http://192.168.4.1`).
3. Go to `Settings` -> `Network`.
4. Enter your home/lab Wi-Fi credentials and save.
5. Reconnect to the rig on your normal network (`scanrig.local` or router IP).

## Hardware Camera Trigger (Relay)

The recommended hardware trigger path is relay-based.

Camera-side wire mapping (confirmed):
- White: `GND`
- Black: `FOCUS`
- Red: `TRIGGER`

Recommended relay wiring:
1. Use two relay channels (or two single-channel relay boards):
   - Relay A = focus
   - Relay B = trigger
2. ESP32 side:
   - Board `IN` pins -> ESP32 trigger GPIOs
   - Board `VCC`/`GND` -> suitable relay supply
   - If your board supports low-level trigger, use active-low logic
3. Camera-side contact wiring:
   - Camera `white (GND)` -> `COM` on both relays
   - Camera `black (FOCUS)` -> `NO` on focus relay
   - Camera `red (TRIGGER)` -> `NO` on trigger relay
   - Leave `NC` unused

Firmware configuration:
- Set trigger pins in `src/secrets.local.h`:
  - `SCANRIG_CAM_FOCUS_PIN`
  - `SCANRIG_CAM_SHUTTER_PIN`
- Keep `SCANRIG_CAM_ACTIVE_LOW 1` for typical low-level-trigger relay modules.
- Tune timings if needed:
  - `SCANRIG_CAM_PREFOCUS_MS`
  - `SCANRIG_CAM_PRESS_MS`

Quick hardware tests (before full scan):
- `SNAP_FOCUS` -> toggles focus relay only
- `SNAP_TRIGGER` -> toggles trigger relay only
- `SNAP` -> full shutter event (focus + trigger sequence in hardware mode)

## Onboard Status LED

The onboard RGB LED is used as a live status indicator with animations:
- Wi-Fi connecting: cyan breathing
- AP fallback: orange double-blink
- Scan running: animated blue/green progress
- Flash guard cooldown: amber pulse
- Paused/recover/error states: dedicated warning/error patterns

Configurable in `src/secrets.local.h`:
- `SCANRIG_STATUS_LED_ENABLE`
- `SCANRIG_STATUS_LED_PIN`
- `SCANRIG_STATUS_LED_BRIGHTNESS`

## Privacy and Credentials

Do not commit real credentials.

- `src/secrets.example.h` is tracked and contains placeholders.
- `src/secrets.local.h` is ignored and intended for private values.

## Developer Notes

For release workflow details, see `RELEASE.md`.

UI automation (render + interaction smoke test):
- Install once: `cmd /c npm install`
- Run against local static UI:  
  `powershell -ExecutionPolicy Bypass -File .\tools\ui-test.ps1 -ServeDocs`
- Run against a live rig UI (recommended for functional checks):  
  `powershell -ExecutionPolicy Bypass -File .\tools\ui-test.ps1 -Url http://scanrig.local`
- Run only device-backed tests (requires reachable rig backend):  
  `powershell -ExecutionPolicy Bypass -File .\tools\ui-test.ps1 -Url http://scanrig.local -DeviceOnly`

What it checks:
- UI renders and key sections are visible
- Core tab/settings interactions
- Broad UI coverage (Control + Presets + Parameters + Firmware section)
- Dirty-state indicator behavior
- Settings layout sanity (no overlap between dirty hint and save row)
- Conditional start warning when turntable is disconnected
- Optional relay diagnostics button clicks (if enabled)
- Device tests (`@device`):
  - `/api/network` + Settings Save/Reload roundtrip with restore
  - Control-flow sanity (presets, trigger toggle, manual action feedback, TT start guard)

Core files:
- Firmware: `src/main.cpp`
- Embedded web assets: `src/web_assets.h`
- Web installer template: `deploy/web-installer/index.html`
- Published installer/OTA artifacts: `docs/`

## Security Reporting

Please report security issues privately first.

See `SECURITY.md` for disclosure instructions.

## Safety Disclaimer

This project is provided as-is and without warranty.

You are responsible for:
- Safe electrical wiring
- Safe operation around moving hardware
- Validation before production use
- Compliance with local regulations

## License

MIT License. See `LICENSE`.
