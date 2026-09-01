#pragma once

/*
Copy this file to `src/secrets.local.h` and fill in real values.
`src/secrets.local.h` is ignored by git.
*/

// mDNS
#define SCANRIG_MDNS_NAME "scanrig"

// AP fallback
#define SCANRIG_AP_SSID "RealityScanRig3000"
#define SCANRIG_AP_PASS "ScanRigPW"

// Default STA profile (used when NVS has no saved credentials yet)
#define SCANRIG_FIRM_SSID "CHANGE_ME_WIFI_SSID"
#define SCANRIG_FIRM_PASS "CHANGE_ME_WIFI_PASS"

// Default static IP profile (set useStatic=true for first boot profile)
#define SCANRIG_FIRM_USE_STATIC 0
#define SCANRIG_FIRM_IP "0.0.0.0"
#define SCANRIG_FIRM_GW "0.0.0.0"
#define SCANRIG_FIRM_DNS "0.0.0.0"
#define SCANRIG_FIRM_MASK "255.255.255.0"

// Hosted manifest URL used by UI "Check updates" action.
// Public default for this project:
// https://superwutz.github.io/RealityScanRig3000/manifest.json
#define SCANRIG_UPDATE_MANIFEST_URL "https://superwutz.github.io/RealityScanRig3000/manifest.json"

// Camera trigger hardware (relay input control pins on ESP32-S3).
// Set to -1 to disable a pin.
//
// Typical relay-board setup:
// - 2x single-channel relay modules (one for focus, one for shutter)
// - IN pin driven by ESP32 GPIO
// - Check your relay board's trigger level: many opto-isolated modules are
//   active-low (set ACTIVE_LOW to 1), some are active-high (set it to 0).
//
// Camera cable mapping used by this project:
// - white = GND
// - black = focus
// - red   = trigger
// Defaults below (ESP32-S3 DevKitC, adapt to your wiring):
// - Focus relay IN   -> GPIO17
// - Trigger relay IN -> GPIO16
// - Active-high trigger (ACTIVE_LOW = 0)
#define SCANRIG_CAM_FOCUS_PIN 17
#define SCANRIG_CAM_SHUTTER_PIN 16
#define SCANRIG_CAM_ACTIVE_LOW 0
#define SCANRIG_CAM_PRESS_MS 120
#define SCANRIG_CAM_PREFOCUS_MS 0

// Onboard status RGB LED (ESP32-S3 DevKitC usually GPIO48 / RGB_BUILTIN).
#define SCANRIG_STATUS_LED_ENABLE 1
#define SCANRIG_STATUS_LED_PIN 48
#define SCANRIG_STATUS_LED_BRIGHTNESS 28
#define SCANRIG_CAM_AF_PREFOCUS_MS 450
#define SCANRIG_CAM_AF_SHUTTER_MS 180
#define SCANRIG_CAM_AF_POSTFOCUS_MS 80

// Smartphone Bluetooth shutter profile name (BLE HID peripheral).
#define SCANRIG_SMARTPHONE_BT_NAME "RealityScanRig3000 Remote"
