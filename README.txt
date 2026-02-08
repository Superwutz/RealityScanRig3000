RealityScanRig3000 — PlatformIO Console-Only Bringup (Firma)
==========================================================

Aktueller Port
--------------
- CH343 (linker USB-C): COM7

Quickstart
----------
1) Projektordner anlegen, Dateien rein:
   - platformio.ini
   - src/main.cpp
2) In VSCode PlatformIO: "Build" und "Upload"
3) Monitor (115200) öffnen

Wenn du nur ROM-Logs siehst:
----------------------------
- Sehr wahrscheinlich hängst du am falschen USB-Port/COM.
- Für Bringup IMMER linker Port (CH343 / UART0) nutzen.

Native USB (rechter Port)
-------------------------
- Nur wenn du env:nativeusb nutzt und den VID 303A COM-Port einträgst.
- Flags sind dort bereits gesetzt.

Erwartete Ausgabe
-----------------
=== RealityScanRig3000 | ESP32-S3 Console-Only Bringup ===
... und dann jede Sekunde:
[1000 ms] tick=1 | free_heap=...

Secrets-Workflow (WLAN/IP)
--------------------------
1) `src/secrets.example.h` nach `src/secrets.local.h` kopieren
2) echte SSID/Passwort/IP in `src/secrets.local.h` eintragen
3) `src/secrets.local.h` bleibt lokal (ist in `.gitignore`)
4) ohne lokale Datei werden Platzhalter aus `src/secrets.example.h` genutzt

Deployment without IDE
----------------------
Goal: ship firmware to users without PlatformIO/Arduino setup.

1) Build release binaries + web installer manifest:
   `powershell -ExecutionPolicy Bypass -File .\tools\export-web-installer.ps1`

2) Output folder:
   `release/web-installer/v-<gitsha>/`
   Contains:
   - `bootloader.bin`
   - `partitions.bin`
   - `boot_app0.bin`
   - `firmware.bin`
   - `manifest.json`

3) Host these files on static hosting (GitHub Pages, Netlify, etc).
   The installer page template is in:
   `deploy/web-installer/index.html`
   Point it to the hosted `manifest.json`.

4) OTA updates from UI:
   Users can later upload a new `firmware.bin` directly in the web UI
   (Console card: "Upload Firmware (.bin)"), no IDE required.
