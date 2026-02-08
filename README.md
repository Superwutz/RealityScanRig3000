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

### 1) First install via browser (USB)

You (or the project maintainer) host:
- `manifest.json`
- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`
- installer page (`index.html`)

User flow:
1. Open installer page in Chrome/Edge
2. Connect board via USB
3. Click install
4. Open rig UI (`http://scanrig.local` or router-assigned IP)

---

## OTA Updates (From Rig UI)

After first install, updates can be done from the UI:

1. Open rig UI
2. Console section:
   - `Check Updates` (manifest-based)
   - `Update Now` (downloads `firmware.bin` and uploads OTA)
3. Device reboots automatically after successful update

You can still upload a local `.bin` manually via `Upload Firmware (.bin)`.

---

## Local Staging Test (Manifest + Installer)

If rig UI runs on another origin (for example `http://scanrig.local`) and manifest is served from `localhost`, your local server must allow CORS.

Example (PowerShell):

```powershell
cd .\.staging\web-installer\v0.4.2
@'
import http.server, socketserver
class H(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()
socketserver.TCPServer.allow_reuse_address = True
socketserver.TCPServer(("", 8000), H).serve_forever()
'@ | & "C:\Users\user\.platformio\python3\python.exe" -
```

Then use:
- `http://localhost:8000`
- manifest URL: `http://localhost:8000/manifest.json`

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
