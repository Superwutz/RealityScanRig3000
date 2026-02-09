# RealityScanRig3000

**RealityScanRig3000** is an ESP32-S3 powered scanning rig controller for automated photogrammetry capture workflows.

Created and maintained by **Superwutz**.

This project was built **largely in collaboration with AI** (design, coding, refactoring, and documentation), with human validation and iterative testing.

---

## What This Project Does

RealityScanRig3000 controls a dual-axis turntable and camera trigger workflow through a browser-based UI:

- Rotation + tilt scan sequencing
- Preset-based scan profiles
- Live status, progress, and segment visualization
- Manual axis controls
- BLE auto-recovery during scan interruptions
- OTA firmware update directly from the UI

The goal is practical, repeatable, low-friction scanning for end users without requiring a full embedded toolchain.

Primary target hardware:
- **Revopoint Dual-axis Turntable**  
  https://www.revopoint3d.com/products/dual-axis-turntable

---

## Key Features

- **Browser UI on device** (ESP32 serves HTML/CSS/JS itself)
- **Dual-axis scan sequencer** with tilt/rotation step logic
- **Live progress views**:
  - top-down rotation ring
  - side-view tilt arc
  - compressed full-sequence bar
- **Error segment marking** in progress views (BLE/TCP/WiFi issues)
- **Manual control block** (rotation/tilt/zero/stop/snap)
- **BLE auto-connect + heartbeat + recover**
- **OTA update endpoint** (`/api/update`)
- **Web-installer export flow** for non-technical end users

---

## Hardware + Software Stack

- **MCU**: ESP32-S3 (`esp32-s3-devkitc-1` target)
- **Framework**: Arduino (PlatformIO)
- **BLE**: NimBLE-Arduino
- **Web server**: ESPAsyncWebServer + AsyncTCP
- **Frontend**: Embedded static assets in firmware (`src/web_assets.h`)

---

## End User Quick Start (No IDE)

### 1) Install via browser (USB)

Use the official installer page:

https://superwutz.github.io/RealityScanRig3000/

User flow:
1. Open the installer page in Chrome or Edge (desktop).
2. Connect your ESP32 board with a USB data cable.
3. Click `Install` and select the board's serial device.
4. Wait until flashing finishes and the board reboots.
5. Open rig UI at `http://scanrig.local` (or router-assigned DHCP IP).

---

## OTA Updates (From Rig UI)

After first install, updates can be done from the UI:

1. Open rig UI
2. Console section:
   - `Check Updates` (manifest-based)
   - `Update Now` (downloads `firmware.bin` and uploads OTA)
3. Device reboots automatically after successful update

You can still upload a local `.bin` manually via `Upload Firmware (.bin)`.

Default OTA manifest endpoint in firmware:

`https://superwutz.github.io/RealityScanRig3000/manifest.json`

If you need a custom endpoint, override `SCANRIG_UPDATE_MANIFEST_URL` in `src/secrets.local.h`.

Release policy and commands are documented in `RELEASE.md`.

---

---

## WiFi + Secrets

Do **not** commit real credentials.

Use:
- `src/secrets.example.h` (placeholders, tracked)
- `src/secrets.local.h` (real values, ignored)

---

## Technical Notes

- Core firmware lives in `src/main.cpp`
- Embedded UI lives in:
  - source workspace: `.tmp_ui/`
  - compiled/embedded form: `src/web_assets.h`
- UI and firmware communicate via WebSocket (`/`) using compact JSON messages
- Sequencer state machine handles:
  - tilt send/wait
  - rotate send/wait
  - settle/snap/cooldown
  - recover on disconnect

---

## Disclaimers

### Safety + Liability

This project is provided **as-is**, without warranty of any kind.  
Use at your own risk.

You are responsible for:
- safe electrical wiring
- safe operation around moving hardware
- protecting camera and rig equipment
- validating all firmware behavior before production use

Neither **Superwutz** nor contributors are liable for equipment damage, data loss, injury, or any consequential damages.

### Compliance

You are responsible for complying with local laws and regulations regarding:
- electronics operation
- radio/BLE usage
- photography and data capture

### Professional Use

This is not a certified industrial safety controller.  
Do not use it where hardware failure can create safety-critical outcomes.

---

## Credits

- Project lead: **Superwutz**
- Built largely with AI-assisted development workflows
- Open-source libraries: NimBLE-Arduino, ESPAsyncWebServer, AsyncTCP, PlatformIO ecosystem

---

## License

This project is licensed under the **MIT License**.  
See `LICENSE` for the full text.
