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
