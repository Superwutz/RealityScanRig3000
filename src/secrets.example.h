#pragma once

/*
Copy this file to `src/secrets.local.h` and fill in real values.
`src/secrets.local.h` is ignored by git.
*/

// mDNS
#define SCANRIG_MDNS_NAME "scanrig"

// AP fallback
#define SCANRIG_AP_SSID "RealityScanRig3000"
#define SCANRIG_AP_PASS "change-me-ap-pass"

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
