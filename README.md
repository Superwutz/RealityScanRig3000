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
- Optional ring pause: the sequence pauses after every tilt move so the camera can be refocused, RESUME starts the ring
- Live status, progress, and error indicators
- Manual motion controls
- HTTP remote-control API for scripts and automation (status, commands, event log)
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

Firmware configuration (no rebuild needed: enter these in the UI's parameter
field or send them through the [Remote Control API](#remote-control-api-http);
they are saved on the board):
- Trigger pins: `SET FOCUS_PIN=17`, `SET SHUTTER_PIN=16` (defaults GPIO17/16)
- Relay logic: `SET CAM_ACTIVE_LOW=1` for the common active-low
  (low-level-trigger) modules, `0` for active-high boards (default `0`)
- Timings if needed: `SNAP_PRESS_MS`, `MANUAL_PREFOCUS_MS` (manual focus) and
  `AF_PREFOCUS_MS`, `AF_SHUTTER_MS`, `AF_POSTFOCUS_MS` (autofocus)

When building yourself, the defaults come from `src/secrets.local.h`
(`SCANRIG_CAM_FOCUS_PIN`, `SCANRIG_CAM_SHUTTER_PIN`, `SCANRIG_CAM_ACTIVE_LOW`,
`SCANRIG_CAM_PRESS_MS`, `SCANRIG_CAM_PREFOCUS_MS`, see
`src/secrets.example.h`); values saved on the board take precedence.

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

## Remote Control API (HTTP)

Everything the UI can do is also available over plain HTTP, so a script, a
home-automation system or an AI agent on your network can run the rig. The
commands are the same text commands the UI sends over its WebSocket.

| Endpoint | Purpose |
|---|---|
| `GET /api/status` | Current state: `rigState` (same fields the UI shows, all values as strings), `logSeq`, `uptimeMs` |
| `POST /api/cmd` | Run one command, parameter `line`. Returns the log lines it produced (`entries`) and the new `rigState` |
| `GET /api/log?since=N` | Log events with sequence number greater than `N` (the last 128 are kept in RAM) |

Examples (replace `scanrig.local` with the IP if mDNS does not resolve):

```bash
curl http://scanrig.local/api/status
curl -X POST http://scanrig.local/api/cmd --data-urlencode "line=SET ROT_STEPS=48"
curl -X POST http://scanrig.local/api/cmd --data-urlencode "line=START"
curl "http://scanrig.local/api/log?since=0"
```

Commands for `line`:
- Sequence: `START`, `PAUSE`, `RESUME`, `ABORT`, `RESET`, `STATUS`
- Parameters: `SET KEY=VALUE`, see the table below. Every key except the
  Wi-Fi credentials is also a field of `rigState`, so you can read back what
  is set.
- Camera: `SNAP`, `SNAP_FOCUS`, `SNAP_TRIGGER`
- Manual motion: `TT_LEFT`, `TT_RIGHT`, `TT_STOP`, `TT_ROT_ZERO`, `TT_TILT_UP`,
  `TT_TILT_DOWN`, `TT_TILT_ZERO`
- Modes: `AF_MODE_AUTO`/`AF_MODE_MANUAL`, `RING_PAUSE_ON`/`RING_PAUSE_OFF`,
  `FLASH_GUARD_ON`/`FLASH_GUARD_OFF`, `TRIGGER_MODE_HW`/`TRIGGER_MODE_SMARTPHONE`
- Turntable: `BLE_CONNECT`, `BLE_DISCONNECT`, `RAW <turntable command>`

Parameters for `SET KEY=VALUE` ("saved" = kept across reboots, otherwise the
value resets to its default on reboot):

| Key | Meaning | Notes |
|---|---|---|
| `ROT_STEPS`, `TILT_STEPS` | Shots per ring, number of rings | only while idle |
| `TILT_FROM`, `TILT_TO` | Tilt of the first and last ring (degrees) | only while idle |
| `TILT_MOVE_MS`, `TILT_RESERVE_MS` | Wait after a tilt move | |
| `SNAP_SETTLE_MS`, `SNAP_COOLDOWN_MS` | Wait before / after each shot | |
| `ROT_TOL_DEG`, `ROT_TIMEOUT_MS`, `POLL_MS` | Rotation target tolerance, timeout, angle polling interval | |
| `FLASH_GUARD`, `FLASH_GUARD_EVERY`, `FLASH_GUARD_MS` | Flash cooldown pause on/off, every N shots, length | not while running |
| `RING_PAUSE` | Pause after every tilt move (`1`/`0`) | saved |
| `AF_MODE` | `AUTO` or `MANUAL` focus | saved, not while running |
| `TRIGGER_MODE` | `HARDWARE` or `SMARTPHONE` | saved, not while running |
| `TRIGGER_ENABLED` | `0` = dry run without shutter | |
| `SNAP_PRESS_MS`, `MANUAL_PREFOCUS_MS` | Manual focus: shutter press length, half-press lead | saved, 0-5000 ms |
| `AF_PREFOCUS_MS`, `AF_SHUTTER_MS`, `AF_POSTFOCUS_MS` | Autofocus: half-press lead, shutter press, half-press hold after | saved, 0-5000 ms |
| `FOCUS_PIN`, `SHUTTER_PIN` | Relay GPIOs (`-1` = unused) | saved, only while idle, safe GPIOs only |
| `CAM_ACTIVE_LOW` | `1` for low-level-trigger relay boards | saved, only while idle |
| `LED_ENABLE`, `LED_BRIGHTNESS` | Status LED on/off, brightness 1-255 | saved |
| `WIFI_SSID`, `WIFI_PASS` | Wi-Fi credentials (applied on reboot) | saved |

For the trigger timings, pins and `CAM_ACTIVE_LOW`, the value `DEFAULT`
(`SET SNAP_PRESS_MS=DEFAULT`) removes the saved value and returns to the
firmware default. An unknown key is answered with `[SET] unknown key` in the
log. Static IP settings are set with `POST /api/network` (fields `ssid`,
`useStatic`, `ip`, `gw`, `dns`, `mask`, `reboot`).

Following a scan:
- `rigState.STATE` is `IDLE`, `RUNNING` or `PAUSED`; `rigState.STEP` is
  `done/total` shots.
- With ring pause on, the rig stops at every ring with `STATE` = `PAUSED` and
  the log line `[SEQ] RING PAUSE at tilt ...`; send `RESUME` to shoot the ring.
- The end of a scan is logged as `[SEQ] DONE`.
- Poll `/api/log?since=<last logSeq>` to read only new lines. If the response
  says `"truncated": true`, lines were lost (more than 128 since your last
  poll, or the rig rebooted).
- Commands sent through the API appear in the UI log as `[API] <command>`.

Security: like the web UI, the API has no password. Only use the rig on a
network you trust. `POST /api/cmd` refuses requests that a browser sends from a
different web page (foreign `Origin` header), so other websites cannot control
the rig through your browser.

## Privacy and Credentials

Do not commit real credentials.

- `src/secrets.example.h` is tracked and contains placeholders.
- `src/secrets.local.h` is ignored and intended for private values.

## Developer Notes

For release workflow details, see `RELEASE.md`.

### Web UI sources

The browser UI is developed as plain files in `web/`
(`index.html`, `style.css`, `app.js`). The firmware serves them from the
generated header `src/web_assets.h` - do not edit that header by hand.

After changing anything in `web/`, regenerate the header:

```
python tools/webassets.py embed
```

`python tools/webassets.py check` verifies header and sources are in sync
(CI runs this on every push).

### UI tests

UI automation (render + interaction smoke test):
- Install once: `npm install`
- Run against the local UI sources (any OS):
  `python3 -m http.server 8080 --directory web` and in a second shell
  `UI_URL=http://127.0.0.1:8080 npx playwright test`
- Windows helper: `powershell -ExecutionPolicy Bypass -File .\tools\ui-test.ps1 -ServeDocs`
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
- Web UI sources: `web/` (embedded via generated `src/web_assets.h`)
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
