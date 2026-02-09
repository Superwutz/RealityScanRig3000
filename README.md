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

If STA connection is unavailable, the device can fall back to AP mode.

## Privacy and Credentials

Do not commit real credentials.

- `src/secrets.example.h` is tracked and contains placeholders.
- `src/secrets.local.h` is ignored and intended for private values.

## Developer Notes

For release workflow details, see `RELEASE.md`.

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
