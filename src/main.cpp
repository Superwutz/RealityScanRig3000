#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "web_assets.h"
#if __has_include("secrets.local.h")
#include "secrets.local.h"
#else
#include "secrets.example.h"
#endif

#ifndef SCANRIG_FW_VERSION
#define SCANRIG_FW_VERSION "0.6.2"
#endif
#ifndef SCANRIG_UI_VERSION
#define SCANRIG_UI_VERSION "0.6.2"
#endif
#ifndef SCANRIG_UPDATE_MANIFEST_URL
#define SCANRIG_UPDATE_MANIFEST_URL ""
#endif
#ifndef SCANRIG_SMARTPHONE_BT_NAME
#define SCANRIG_SMARTPHONE_BT_NAME "RealityScanRig3000 Remote"
#endif
#ifndef SCANRIG_CAM_FOCUS_PIN
#define SCANRIG_CAM_FOCUS_PIN -1
#endif
#ifndef SCANRIG_CAM_SHUTTER_PIN
#define SCANRIG_CAM_SHUTTER_PIN -1
#endif
#ifndef SCANRIG_CAM_ACTIVE_LOW
#define SCANRIG_CAM_ACTIVE_LOW 1
#endif
#ifndef SCANRIG_CAM_PRESS_MS
#define SCANRIG_CAM_PRESS_MS 120
#endif
#ifndef SCANRIG_CAM_PREFOCUS_MS
#define SCANRIG_CAM_PREFOCUS_MS 0
#endif
#ifndef SCANRIG_STATUS_LED_ENABLE
#define SCANRIG_STATUS_LED_ENABLE 1
#endif
#ifndef SCANRIG_STATUS_LED_PIN
#ifdef RGB_BUILTIN
#define SCANRIG_STATUS_LED_PIN RGB_BUILTIN
#else
#define SCANRIG_STATUS_LED_PIN 48
#endif
#endif
#ifndef SCANRIG_STATUS_LED_BRIGHTNESS
#define SCANRIG_STATUS_LED_BRIGHTNESS 28
#endif
#ifndef SCANRIG_BUILD_GIT
#define SCANRIG_BUILD_GIT "dev"
#endif
#ifndef SCANRIG_CAM_AF_PREFOCUS_MS
#define SCANRIG_CAM_AF_PREFOCUS_MS 450
#endif
#ifndef SCANRIG_CAM_AF_SHUTTER_MS
#define SCANRIG_CAM_AF_SHUTTER_MS 180
#endif
#ifndef SCANRIG_CAM_AF_POSTFOCUS_MS
#define SCANRIG_CAM_AF_POSTFOCUS_MS 80
#endif

/*
RealityScanRig3000 ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã¢â‚¬Å“ ESP32-S3: UI (WebSocket) + BLE Turntable + Sequencer (v1)

Goal:
- Serve the existing UI 1:1 from the ESP32 (HTML/CSS/JS embedded)
- Provide a WebSocket endpoint at "/" (same host) matching the old Node protocol
- Dock commands to the Sequencer in this firmware

Notes:
- UART0 remains debug lifeline.
- No USB-CDC / no native USB.
- BLE: NimBLE-Arduino 1.4.2 (pinned)
- Web: ESPAsyncWebServer 1.2.3 + AsyncTCP 1.1.1 (pinned)
- mDNS: scanrig.local

Protocol from UI:
- UI connects WS to ws://<host>/  (no explicit path)
- UI sends JSON:
    { "type": "requestStatus" }
    { "type": "cmd", "line": "STATUS" } etc.
- Firmware sends JSON:
    { "type": "hello", "rigState": {...}, "tcpConnected": true }
    { "type": "status", "rigState": {...}, "tcpConnected": true }
    { "type": "log", "msg": "..." }
*/

// ===== WiFi config (stored in NVS) =====
static const char* MDNS_NAME = SCANRIG_MDNS_NAME; // -> <name>.local
static const uint16_t HTTP_PORT = 80;

Preferences prefs;
static String wifiSsid;
static String wifiPass;

// AP fallback
static const char* AP_SSID = SCANRIG_AP_SSID;
static const char* AP_PASS = SCANRIG_AP_PASS;

// ===== Firm profile (project memory) =====
// NOTE: You can overwrite these via Serial commands (stored in NVS).
static const char* FIRM_SSID = SCANRIG_FIRM_SSID;
static const char* FIRM_PASS = SCANRIG_FIRM_PASS;

// Static IP profile (temporary in company network)
static const char* FIRM_IP   = SCANRIG_FIRM_IP;
static const char* FIRM_GW   = SCANRIG_FIRM_GW;
static const char* FIRM_DNS  = SCANRIG_FIRM_DNS;
static const char* FIRM_MASK = SCANRIG_FIRM_MASK;

static bool   wifiUseStatic = (SCANRIG_FIRM_USE_STATIC != 0);
static String wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr;
static const char* SMARTPHONE_BT_NAME = SCANRIG_SMARTPHONE_BT_NAME;
static const int CAM_FOCUS_PIN = SCANRIG_CAM_FOCUS_PIN;
static const int CAM_SHUTTER_PIN = SCANRIG_CAM_SHUTTER_PIN;
static const bool CAM_ACTIVE_LOW = (SCANRIG_CAM_ACTIVE_LOW != 0);
static const uint32_t CAM_PRESS_MS = SCANRIG_CAM_PRESS_MS;
static const uint32_t CAM_PREFOCUS_MS = SCANRIG_CAM_PREFOCUS_MS;
static const bool STATUS_LED_ENABLE_DEFAULT = (SCANRIG_STATUS_LED_ENABLE != 0);
static const uint8_t STATUS_LED_PIN = (uint8_t)SCANRIG_STATUS_LED_PIN;
static const uint8_t STATUS_LED_BRIGHTNESS_DEFAULT = (uint8_t)SCANRIG_STATUS_LED_BRIGHTNESS;
static const int STATUS_LED_BRIGHTNESS_MIN = 1;
static const int STATUS_LED_BRIGHTNESS_MAX = 255;
static bool gStatusLedEnabled = STATUS_LED_ENABLE_DEFAULT;
static uint8_t gStatusLedBrightness = STATUS_LED_BRIGHTNESS_DEFAULT;
static String gWifiRuntimeMode = "INIT";
static String gWifiLastError = "";

struct LedRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
static uint32_t gLedShotFlashUntilMs = 0;
static uint32_t gLedVictoryUntilMs = 0;
static uint32_t gLedLastDoneSteps = 0;
static uint8_t gLedPrevSeqState = 0;

static inline uint8_t ledScale(uint8_t c, uint8_t brightness) {
  return (uint8_t)(((uint16_t)c * (uint16_t)brightness) / 255u);
}
static inline void ledWriteRaw(uint8_t r, uint8_t g, uint8_t b) {
  if (!gStatusLedEnabled) return;
  neopixelWrite(STATUS_LED_PIN, ledScale(r, gStatusLedBrightness), ledScale(g, gStatusLedBrightness), ledScale(b, gStatusLedBrightness));
}
static inline LedRgb ledBlend(const LedRgb& a, const LedRgb& b, float t) {
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  LedRgb c;
  c.r = (uint8_t)(a.r + (b.r - a.r) * t);
  c.g = (uint8_t)(a.g + (b.g - a.g) * t);
  c.b = (uint8_t)(a.b + (b.b - a.b) * t);
  return c;
}
static inline uint8_t ledPulse8(uint32_t nowMs, uint16_t periodMs, uint8_t minVal, uint8_t maxVal) {
  if (periodMs < 2) periodMs = 2;
  uint32_t m = nowMs % periodMs;
  float t = (float)m / (float)periodMs; // 0..1
  float tri = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
  return (uint8_t)(minVal + (uint8_t)((maxVal - minVal) * tri));
}
static void ledBootWifiConnectingTick() {
  if (!gStatusLedEnabled) return;
  uint8_t p = ledPulse8(millis(), 1000, 6, 90);
  ledWriteRaw(0, p, p);
}
static const uint32_t CAM_AF_PREFOCUS_MS = SCANRIG_CAM_AF_PREFOCUS_MS;
static const uint32_t CAM_AF_SHUTTER_MS = SCANRIG_CAM_AF_SHUTTER_MS;
static const uint32_t CAM_AF_POSTFOCUS_MS = SCANRIG_CAM_AF_POSTFOCUS_MS;

enum TriggerMode : uint8_t {
  TRIGGER_MODE_HW = 0,
  TRIGGER_MODE_SMARTPHONE = 1,
};
static TriggerMode gTriggerMode = TRIGGER_MODE_HW;
static TriggerMode gSeqTriggerMode = TRIGGER_MODE_HW;
static bool gTriggerEnabled = true;
static bool gAutoFocusEnabled = true;

static NimBLEServer* gPhoneServer = nullptr;
static NimBLEHIDDevice* gPhoneHid = nullptr;
static NimBLECharacteristic* gPhoneInput = nullptr;
static bool gPhoneConnected = false;
static bool gPhonePairing = false;
static String gPhonePeerName = "";
static bool gNeedPhoneRecoverOnConnect = false;
static bool gBleAutoConnect = true;
static volatile bool gPhoneConnectSeen = false;
static volatile bool gPhoneDisconnectSeen = false;
static volatile bool gReqPhonePairStart = false;
static volatile bool gReqPhonePairStop = false;
static void bleDisconnect();

static bool parseIP(const String& s, IPAddress& out) {
  int a, b, c, d;
  if (sscanf(s.c_str(), "%d.%d.%d.%d", &a,&b,&c,&d) != 4) return false;
  if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return false;
  out = IPAddress(a,b,c,d);
  return true;
}


static void loadWifiCreds() {
  prefs.begin("scanrig", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");

  wifiUseStatic = prefs.getBool("useStatic", false);
  wifiIpStr   = prefs.getString("ip", "");
  wifiGwStr   = prefs.getString("gw", "");
  wifiDnsStr  = prefs.getString("dns", "");
  wifiMaskStr = prefs.getString("mask", "");
  prefs.end();
}

static void saveWifiCreds(const String& ssid, const String& pass) {
  prefs.begin("scanrig", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

static void saveWifiStatic(bool useStatic, const String& ip, const String& gw, const String& dns, const String& mask) {
  prefs.begin("scanrig", false);
  prefs.putBool("useStatic", useStatic);
  prefs.putString("ip", ip);
  prefs.putString("gw", gw);
  prefs.putString("dns", dns);
  prefs.putString("mask", mask);
  prefs.end();
}

static void loadTriggerMode() {
  prefs.begin("scanrig", true);
  int mode = prefs.getInt("trigMode", (int)TRIGGER_MODE_HW);
  prefs.end();
  gTriggerMode = (mode == (int)TRIGGER_MODE_SMARTPHONE) ? TRIGGER_MODE_SMARTPHONE : TRIGGER_MODE_HW;
}

static void saveTriggerMode(TriggerMode mode) {
  prefs.begin("scanrig", false);
  prefs.putInt("trigMode", (int)mode);
  prefs.end();
}

static void loadAutoFocusMode() {
  prefs.begin("scanrig", true);
  gAutoFocusEnabled = prefs.getBool("afMode", true);
  prefs.end();
}

static void saveAutoFocusMode(bool enabled) {
  prefs.begin("scanrig", false);
  prefs.putBool("afMode", enabled);
  prefs.end();
}

static void loadStatusLedConfig() {
  prefs.begin("scanrig", true);
  gStatusLedEnabled = prefs.getBool("ledEn", STATUS_LED_ENABLE_DEFAULT);
  int b = prefs.getInt("ledBr", (int)STATUS_LED_BRIGHTNESS_DEFAULT);
  prefs.end();
  if (b < STATUS_LED_BRIGHTNESS_MIN) b = STATUS_LED_BRIGHTNESS_MIN;
  if (b > STATUS_LED_BRIGHTNESS_MAX) b = STATUS_LED_BRIGHTNESS_MAX;
  gStatusLedBrightness = (uint8_t)b;
}

static void saveStatusLedConfig(bool enabled, uint8_t brightness) {
  prefs.begin("scanrig", false);
  prefs.putBool("ledEn", enabled);
  prefs.putInt("ledBr", (int)brightness);
  prefs.end();
}

static void wifiStart() {
  loadWifiCreds();
  gWifiRuntimeMode = "INIT";
  gWifiLastError = "";

  // If nothing saved yet, default to firm profile (can be changed via Serial).
  if (!wifiSsid.length()) {
    wifiSsid = FIRM_SSID;
    wifiPass = FIRM_PASS;
    wifiUseStatic = (SCANRIG_FIRM_USE_STATIC != 0);
    wifiIpStr = FIRM_IP;
    wifiGwStr = FIRM_GW;
    wifiDnsStr = FIRM_DNS;
    wifiMaskStr = FIRM_MASK;
    saveWifiCreds(wifiSsid, wifiPass);
    saveWifiStatic(wifiUseStatic, wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr);
    Serial.println("[WIFI] no saved creds -> defaulting to FIRM profile from secrets");
  }

  WiFi.mode(WIFI_STA);

  if (wifiUseStatic) {
    IPAddress ip, gw, dns, mask;
    if (parseIP(wifiIpStr, ip) && parseIP(wifiGwStr, gw) && parseIP(wifiDnsStr, dns) && parseIP(wifiMaskStr, mask)) {
      // WiFi.config(local_ip, gateway, subnet, dns1, dns2)
      WiFi.config(ip, gw, mask, dns, dns);
      Serial.printf("[WIFI] static IP enabled: ip=%s gw=%s mask=%s dns=%s\n",
                    wifiIpStr.c_str(), wifiGwStr.c_str(), wifiMaskStr.c_str(), wifiDnsStr.c_str());
    } else {
      Serial.println("[WIFI] static IP config invalid -> using DHCP");
    }
  } else {
    Serial.println("[WIFI] DHCP enabled");
  }

  if (wifiSsid.length()) {
    Serial.printf("[WIFI] STA connect ssid='%s'\n", wifiSsid.c_str());
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
      ledBootWifiConnectingTick();
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] connected IP=%s\n", WiFi.localIP().toString().c_str());
      gWifiRuntimeMode = "STA";
      return;
    }
    Serial.println("[WIFI] STA connect failed -> AP fallback");
    gWifiLastError = "STA connect failed; AP fallback enabled";
  } else {
    Serial.println("[WIFI] no ssid -> AP fallback");
    gWifiLastError = "No SSID configured; AP fallback enabled";
  }

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WIFI] AP %s ssid='%s' pass='%s'\n", ok ? "ON" : "FAIL", AP_SSID, AP_PASS);
  Serial.printf("[WIFI] AP IP=%s\n", WiFi.softAPIP().toString().c_str());
  gWifiRuntimeMode = ok ? "AP" : "AP_FAIL";
  if (!ok && !gWifiLastError.length()) {
    gWifiLastError = "AP startup failed";
  }
}

static void mdnsStart() {
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("[mDNS] %s.local ready\n", MDNS_NAME);
  } else {
    Serial.println("[mDNS] start failed");
  }
}

// ===== Web server + WS =====
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/"); // UI expects ws://host/ // IMPORTANT: path "/" so the UI's ws://host works

static void wsBroadcastJson(const String& json) {
  ws.textAll(json);
}
static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\\') out += "\\\\";
    else if (c == '\"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') {}
    else out += c;
  }
  return out;
}

static String getPostParam(AsyncWebServerRequest* req, const char* key, const String& def = "") {
  if (!req->hasParam(key, true)) return def;
  const AsyncWebParameter* p = req->getParam(key, true);
  if (!p) return def;
  return p->value();
}

static bool parseBoolParam(const String& v) {
  String s = v;
  s.trim();
  s.toLowerCase();
  return (s == "1" || s == "true" || s == "on" || s == "yes");
}

static String currentIpText() {
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

static String wifiModeText() {
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) return "STA";
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) return "AP";
  return gWifiRuntimeMode;
}

static String buildApiErrorJson(const String& code, const String& message) {
  String body = "{";
  body += "\"ok\":false,";
  body += "\"code\":\"" + jsonEscape(code) + "\",";
  body += "\"message\":\"" + jsonEscape(message) + "\",";
  body += "\"msg\":\"" + jsonEscape(message) + "\"";
  body += "}";
  return body;
}

static String buildNetworkConfigJson(const String& msg = "", const String& code = "OK", const String& savedFieldsJson = "[]") {
  String j = "{";
  j += "\"ok\":true,";
  j += "\"code\":\"" + jsonEscape(code) + "\",";
  j += "\"message\":\"" + jsonEscape(msg) + "\",";
  j += "\"msg\":\"" + jsonEscape(msg) + "\",";
  j += "\"ssid\":\"" + jsonEscape(wifiSsid) + "\",";
  j += "\"passSet\":" + String(wifiPass.length() ? 1 : 0) + ",";
  j += "\"useStatic\":" + String(wifiUseStatic ? 1 : 0) + ",";
  j += "\"ip\":\"" + jsonEscape(wifiIpStr) + "\",";
  j += "\"gw\":\"" + jsonEscape(wifiGwStr) + "\",";
  j += "\"dns\":\"" + jsonEscape(wifiDnsStr) + "\",";
  j += "\"mask\":\"" + jsonEscape(wifiMaskStr) + "\",";
  j += "\"ledEnable\":" + String(gStatusLedEnabled ? 1 : 0) + ",";
  j += "\"ledBrightness\":" + String((int)gStatusLedBrightness) + ",";
  j += "\"ledBrightnessMin\":" + String(STATUS_LED_BRIGHTNESS_MIN) + ",";
  j += "\"ledBrightnessMax\":" + String(STATUS_LED_BRIGHTNESS_MAX) + ",";
  j += "\"mdns\":\"" + jsonEscape(String(MDNS_NAME)) + "\",";
  j += "\"currentIp\":\"" + jsonEscape(currentIpText()) + "\",";
  j += "\"wifiMode\":\"" + jsonEscape(wifiModeText()) + "\",";
  j += "\"apSsid\":\"" + jsonEscape(String(AP_SSID)) + "\",";
  j += "\"apPassSet\":" + String(String(AP_PASS).length() ? 1 : 0) + ",";
  j += "\"wifiLastError\":\"" + jsonEscape(gWifiLastError) + "\",";
  j += "\"savedFields\":" + savedFieldsJson;
  j += "}";
  return j;
}

static bool validateStaticConfig(const String& ip, const String& gw, const String& dns, const String& mask, String& err) {
  IPAddress a, b, c, d;
  if (!parseIP(ip, a))   { err = "invalid static IP"; return false; }
  if (!parseIP(gw, b))   { err = "invalid gateway IP"; return false; }
  if (!parseIP(dns, c))  { err = "invalid DNS IP"; return false; }
  if (!parseIP(mask, d)) { err = "invalid subnet mask"; return false; }
  return true;
}

static inline uint8_t camIdleLevel() { return CAM_ACTIVE_LOW ? HIGH : LOW; }
static inline uint8_t camActiveLevel() { return CAM_ACTIVE_LOW ? LOW : HIGH; }

class PhoneHidServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
    (void)s;
    if (desc) {
      NimBLEAddress idAddr(desc->peer_id_addr);
      gPhonePeerName = idAddr.toString().c_str();
    } else {
      gPhonePeerName = "";
    }
    gPhoneConnected = true;
    gPhonePairing = false;
    gBleAutoConnect = true;
    gPhoneConnectSeen = true;
  }
  void onConnect(NimBLEServer* s) override {
    (void)s;
    if (!gPhoneConnected) {
      gPhoneConnected = true;
      gPhonePairing = false;
      gBleAutoConnect = true;
      gPhoneConnectSeen = true;
    }
  }
  void onDisconnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
    (void)desc;
    gPhoneConnected = false;
    gPhonePeerName = "";
    gPhoneDisconnectSeen = true;
    if (gPhonePairing && s && s->getAdvertising()) {
      s->getAdvertising()->start();
    }
  }
  void onDisconnect(NimBLEServer* s) override {
    gPhoneConnected = false;
    gPhonePeerName = "";
    gPhoneDisconnectSeen = true;
    if (gPhonePairing && s && s->getAdvertising()) {
      s->getAdvertising()->start();
    }
  }
};
static PhoneHidServerCallbacks gPhoneHidCbs;

static bool triggerCameraHardware() {
  if (!gTriggerEnabled) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] skipped (dry-run enabled)\"}");
    return true;
  }
  if (CAM_SHUTTER_PIN < 0) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] hardware trigger disabled (no shutter pin)\"}");
    return false;
  }

  if (gAutoFocusEnabled) {
    if (CAM_FOCUS_PIN >= 0) {
      // Nikon-style sequence for AF shutter release:
      // 1) hold focus, 2) add shutter, 3) release shutter, 4) release focus.
      digitalWrite(CAM_FOCUS_PIN, camActiveLevel());
      if (CAM_AF_PREFOCUS_MS > 0) delay(CAM_AF_PREFOCUS_MS);
      digitalWrite(CAM_SHUTTER_PIN, camActiveLevel());
      delay(CAM_AF_SHUTTER_MS);
      digitalWrite(CAM_SHUTTER_PIN, camIdleLevel());
      if (CAM_AF_POSTFOCUS_MS > 0) delay(CAM_AF_POSTFOCUS_MS);
      digitalWrite(CAM_FOCUS_PIN, camIdleLevel());
    } else {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus requested but no focus pin -> shutter only\"}");
      digitalWrite(CAM_SHUTTER_PIN, camActiveLevel());
      delay(CAM_AF_SHUTTER_MS);
      digitalWrite(CAM_SHUTTER_PIN, camIdleLevel());
    }
  } else {
    if (CAM_FOCUS_PIN >= 0) {
      digitalWrite(CAM_FOCUS_PIN, camActiveLevel());
      if (CAM_PREFOCUS_MS > 0) delay(CAM_PREFOCUS_MS);
    }
    digitalWrite(CAM_SHUTTER_PIN, camActiveLevel());
    delay(CAM_PRESS_MS);
    digitalWrite(CAM_SHUTTER_PIN, camIdleLevel());
    if (CAM_FOCUS_PIN >= 0) {
      digitalWrite(CAM_FOCUS_PIN, camIdleLevel());
    }
  }
  return true;
}

static bool triggerCameraHardwareFocusOnly() {
  if (!gTriggerEnabled) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] focus skipped (dry-run enabled)\"}");
    return true;
  }
  if (CAM_FOCUS_PIN < 0) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] focus trigger disabled (no focus pin)\"}");
    return false;
  }
  digitalWrite(CAM_FOCUS_PIN, camActiveLevel());
  delay(CAM_PRESS_MS);
  digitalWrite(CAM_FOCUS_PIN, camIdleLevel());
  return true;
}

static bool triggerCameraHardwareShutterOnly() {
  if (!gTriggerEnabled) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger skipped (dry-run enabled)\"}");
    return true;
  }
  if (CAM_SHUTTER_PIN < 0) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] shutter trigger disabled (no shutter pin)\"}");
    return false;
  }
  digitalWrite(CAM_SHUTTER_PIN, camActiveLevel());
  delay(CAM_PRESS_MS);
  digitalWrite(CAM_SHUTTER_PIN, camIdleLevel());
  return true;
}

static void phonePairingStart() {
  if (!gPhoneServer || !gPhoneServer->getAdvertising()) return;
  bleDisconnect();
  gPhonePairing = true;
  NimBLEAdvertising* adv = gPhoneServer->getAdvertising();
  bool ok = adv->isAdvertising() ? true : adv->start();
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[PHONE] pairing enabled (advertising ")
                  + (ok ? "OK" : "FAIL") + ")\"}");
}

static void phonePairingStop() {
  if (!gPhoneServer || !gPhoneServer->getAdvertising()) return;
  gPhonePairing = false;
  bool ok = true;
  if (!gPhoneConnected && gPhoneServer->getAdvertising()->isAdvertising()) {
    ok = gPhoneServer->getAdvertising()->stop();
  }
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[PHONE] pairing disabled (")
                  + (ok ? "OK" : "FAIL") + ")\"}");
}

static void phoneDisconnect() {
  if (!gPhoneServer) return;
  gPhonePairing = false;
  gPhoneConnected = false;
  gPhonePeerName = "";
  if (gPhoneServer->getAdvertising() && gPhoneServer->getAdvertising()->isAdvertising()) {
    gPhoneServer->getAdvertising()->stop();
  }
  int n = 0;
  auto peers = gPhoneServer->getPeerDevices();
  for (auto connId : peers) {
    gPhoneServer->disconnect(connId);
    n++;
  }
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[PHONE] disconnect requested (")
                  + String(n) + " peer(s))\"}");
}

static void phoneDisableForHardwareMode() {
  gReqPhonePairStart = false;
  gReqPhonePairStop = false;
  phonePairingStop();
  phoneDisconnect();
}

static bool triggerCameraSmartphone() {
  if (!gTriggerEnabled) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] skipped (dry-run enabled)\"}");
    return true;
  }
  if (!gPhoneConnected || !gPhoneInput) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] smartphone trigger unavailable (not connected)\"}");
    return false;
  }

  // Consumer key: Volume Increment (0x00E9), recognized as shutter by many camera apps.
  uint8_t press[2] = { 0xE9, 0x00 };
  uint8_t release[2] = { 0x00, 0x00 };
  gPhoneInput->setValue(press, sizeof(press));
  gPhoneInput->notify();
  delay(30);
  gPhoneInput->setValue(release, sizeof(release));
  gPhoneInput->notify();
  return true;
}

static bool triggerCamera() {
  if (gTriggerMode == TRIGGER_MODE_SMARTPHONE) return triggerCameraSmartphone();
  return triggerCameraHardware();
}

static void initSmartphoneTriggerBle() {
  static bool inited = false;
  if (inited) return;
  inited = true;

  gPhoneServer = NimBLEDevice::createServer();
  if (!gPhoneServer) return;
  gPhoneServer->setCallbacks(&gPhoneHidCbs);

  gPhoneHid = new NimBLEHIDDevice(gPhoneServer);
  if (!gPhoneHid) return;

  // Single consumer-control input report (volume up shutter pulse).
  static const uint8_t reportMap[] = {
    0x05, 0x0C,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       // Report ID (1)
    0x15, 0x00,       // Logical Minimum (0)
    0x26, 0x9C, 0x02, // Logical Maximum (668)
    0x19, 0x00,       // Usage Minimum (0)
    0x2A, 0x9C, 0x02, // Usage Maximum (668)
    0x75, 0x10,       // Report Size (16)
    0x95, 0x01,       // Report Count (1)
    0x81, 0x00,       // Input (Data, Array, Abs)
    0xC0              // End Collection
  };

  gPhoneInput = gPhoneHid->inputReport(1);
  gPhoneHid->manufacturer()->setValue("Superwutz");
  gPhoneHid->pnp(0x02, 0xe502, 0xa111, 0x0110);
  gPhoneHid->hidInfo(0x00, 0x01);
  gPhoneHid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  gPhoneHid->startServices();
  gPhoneServer->start();

  NimBLEAdvertising* adv = gPhoneServer->getAdvertising();
  if (adv) {
    NimBLEDevice::setDeviceName(SMARTPHONE_BT_NAME);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    adv->setMaxPreferred(0x12);
    adv->setAppearance(0x03C1); // Generic HID
    adv->addServiceUUID(gPhoneHid->hidService()->getUUID());
    adv->setName(SMARTPHONE_BT_NAME);
  }
}

// ===== Turntable BLE =====
static const char* TT_NAME = "REVO_DUAL_AXIS_TABLE";
static const NimBLEUUID TT_SVC_UUID((uint16_t)0xFFE0);
static const NimBLEUUID TT_CHR_UUID((uint16_t)0xFFE1);

static NimBLEClient* gClient = nullptr;
static NimBLERemoteCharacteristic* gChr = nullptr;
static bool gBleConnected = false;
static volatile bool gBleDisconnectSeen = false;
static bool gFwUpdateInProgress = false;
static bool gFwUpdateOk = false;
static String gFwUpdateError = "";
static const char* FW_VERSION = SCANRIG_FW_VERSION;
static const char* UI_VERSION = SCANRIG_UI_VERSION;
static const char* UPDATE_MANIFEST_URL = SCANRIG_UPDATE_MANIFEST_URL;
static const char* BUILD_GIT = SCANRIG_BUILD_GIT;
static const char* BUILD_TIME = __DATE__ " " __TIME__;

class TTClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient*) override {
    gBleConnected = false;
    gChr = nullptr;
    Serial.println("[BLE] disconnected");
    gBleDisconnectSeen = true;
  }
};
static TTClientCallbacks gTtCbs;
static NimBLEAddress gLastTTAddr("");


// RX assembly
static String gAsm;
static volatile bool gAngleUpdated = false;
static float gLastAngle = 0.0f;

static uint32_t gLastRxMs = 0;          // last time we saw any TT notification
static uint32_t gLastHeartbeatMs = 0;   // last time we sent a heartbeat query
static bool gHeartbeatPending = false;  // waiting for a response

static float normAngle360(float a);

static void onNotifyCB(NimBLERemoteCharacteristic*, uint8_t* pData, size_t len, bool) {
  gLastRxMs = millis();
  gHeartbeatPending = false;
  for (size_t i = 0; i < len; i++) {
    char c = (char)pData[i];
    if (c == '\r') continue;

    if (c == '\n' || c == ';') {
      if (gAsm.length()) {
        // Parse +DATA= anywhere
        int p = gAsm.indexOf("+DATA=");
        if (p >= 0) {
          p += 6;
          int e = p;
          while (e < (int)gAsm.length()) {
            char ch = gAsm[e];
            if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '.') e++;
            else break;
          }
          gLastAngle = normAngle360(gAsm.substring(p, e).toFloat());
          gAngleUpdated = true;
        }
      }
      gAsm = "";
    } else {
      gAsm += c;
      if (gAsm.length() > 240) gAsm.remove(0, 120);
    }
  }
}

static void bleInitOnce() {
  static bool inited = false;
  if (inited) return;
  inited = true;

  NimBLEDevice::init("ScanRig3000");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, false, false);
}

static void bleDisconnect() {
  if (gClient && gClient->isConnected()) gClient->disconnect();
  gBleConnected = false;
  gChr = nullptr;
}

static bool bleWriteRaw(const String& s) {
  if (!gBleConnected || !gChr) return false;
  std::string out = s.c_str();
  return gChr->writeValue((uint8_t*)out.data(), out.size(), false);
}

static bool findTTAddr(NimBLEAddress& outAddr) {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(15);

  NimBLEScanResults res = scan->start(4, false);

  for (int i = 0; i < res.getCount(); i++) {
    NimBLEAdvertisedDevice dev = res.getDevice(i);
    if (dev.getName() == TT_NAME) {
      outAddr = dev.getAddress();
      return true;
    }
  }
  for (int i = 0; i < res.getCount(); i++) {
    NimBLEAdvertisedDevice dev = res.getDevice(i);
    String n = dev.getName().c_str();
    if (n.length() && n.indexOf(TT_NAME) >= 0) {
      outAddr = dev.getAddress();
      return true;
    }
  }
  return false;
}

static bool bleConnect() {
  if (gBleConnected) return true;
  bleInitOnce();

  NimBLEAddress addr("");
  if (gLastTTAddr.toString() != "") {
    addr = gLastTTAddr;
  } else {
    if (!findTTAddr(addr)) return false;
  }

  if (!gClient) {
    gClient = NimBLEDevice::createClient();
    gClient->setClientCallbacks(&gTtCbs, false);
  }

  if (!gClient->connect(addr)) {
    if (gLastTTAddr.toString() != "") {
      gLastTTAddr = NimBLEAddress("");
      NimBLEAddress addr2("");
      if (!findTTAddr(addr2)) return false;
      if (!gClient->connect(addr2)) return false;
      addr = addr2;
    } else {
      return false;
    }
  }

  NimBLERemoteService* svc = gClient->getService(TT_SVC_UUID);
  if (!svc) { bleDisconnect(); return false; }

  gChr = svc->getCharacteristic(TT_CHR_UUID);
  if (!gChr) { bleDisconnect(); return false; }

  if (gChr->canNotify()) {
    gChr->subscribe(true, onNotifyCB);
  }

  gLastTTAddr = addr;
  gBleConnected = true;
  gLastRxMs = millis();
  gLastHeartbeatMs = 0;
  gHeartbeatPending = false;
  Serial.println("[BLE] connected");
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[BLE] connected\"}");
  return true;
}

// ===== Sequencer (same logic as v1) =====
enum SeqState : uint8_t { SEQ_IDLE=0, SEQ_RUNNING, SEQ_PAUSED };
enum SeqSub   : uint8_t { SUB_NONE=0, SUB_RECOVER, SUB_RECOVER_WAIT, SUB_ROT_SEEK_WAIT, SUB_TILT_SEND, SUB_TILT_WAIT, SUB_ROT_SEND, SUB_ROT_WAIT, SUB_SETTLE, SUB_SNAP, SUB_COOLDOWN, SUB_DONE, SUB_FLASH_COOLDOWN };

static SeqState gSeqState = SEQ_IDLE;
static SeqSub   gSub = SUB_NONE;

// Defaults (UI can SET these)
static int   gRotSteps  = 72;
static int   gTiltSteps = 9;
static float gTiltFrom  = -30.0f;
static float gTiltTo    = +30.0f;

static uint32_t gPollMs = 150;
static uint32_t gRotTimeoutMs = 8000;
static float    gRotTolDeg = 1.0f;
static uint32_t gSnapSettleMs = 250;
static uint32_t gSnapCooldownMs = 500;
static uint32_t gTiltMoveMs = 8000;
static uint32_t gTiltReserveMs = 0;
static bool     gFlashGuardEnabled = true;
static uint32_t gFlashGuardEveryShots = 200;
static uint32_t gFlashGuardMs = 300000;
static uint32_t gFlashGuardUntilMs = 0;

static int gTiltIdx = 0;
static int gRotIdx  = 0;
static float gStepDeg = 5.0f;

static float gCurTiltTarget = 0.0f;
static float gRotTargetDeg  = 0.0f;
static float gRotRowZeroDeg = 0.0f;
static uint32_t gStateTs = 0;
static uint32_t gNextPollTs = 0;

static uint32_t gTotalSteps = 72 * 9;
static uint32_t gDoneSteps = 0;

static bool gResumePending = false;
static SeqSub gResumeSub = SUB_NONE;

static float gRecoverLastAngle = 0.0f;
static uint32_t gRecoverStableSince = 0;
static bool gRecovering = false;
static uint8_t gRecoverAttempts = 0;
static uint32_t gRecoverStartMs = 0;
static uint32_t gRecoverResendNextMs = 0;
static bool gNeedRecoverOnConnect = false;
static uint32_t gForceDiscNextMs = 0;

static void seqAdvanceAfterShot() {
  gRotIdx++;
  if (gRotIdx < gRotSteps) {
    gSub = SUB_ROT_SEND;
  } else {
    gTiltIdx++;
    if (gTiltIdx < gTiltSteps) {
      gSub = SUB_TILT_SEND;
    } else {
      gSub = SUB_DONE;
    }
  }
}
static bool gRepeatStep = false;
static bool gHasResumeAngle = false;
static float gResumeAngle = 0.0f;
static uint8_t gSeekAttempts = 0;

static uint32_t gIdleMonitorUntilMs = 0;
static uint32_t gIdleNextPollMs = 0;
static float gIdleLastAngle = 0.0f;
static bool gIdleHaveAngle = false;

static uint32_t gRotStepStartMs = 0;
static float gRotStepMsAvg = 0.0f;
static uint32_t gRotStepMsSamples = 0;

static void ledTick(uint32_t nowMs) {
  if (!gStatusLedEnabled) {
    neopixelWrite(STATUS_LED_PIN, 0, 0, 0);
    return;
  }

  // Detect shot progress edges for short white "tick" flashes while running.
  if (gDoneSteps != gLedLastDoneSteps) {
    if (gSeqState == SEQ_RUNNING && gDoneSteps > gLedLastDoneSteps) {
      gLedShotFlashUntilMs = nowMs + 85;
    }
    gLedLastDoneSteps = gDoneSteps;
  }

  // Victory animation trigger when a run transitions to IDLE and completed all steps.
  if (gLedPrevSeqState == (uint8_t)SEQ_RUNNING &&
      gSeqState == SEQ_IDLE &&
      gTotalSteps > 0 &&
      gDoneSteps >= gTotalSteps) {
    gLedVictoryUntilMs = nowMs + 3000;
  }
  gLedPrevSeqState = (uint8_t)gSeqState;

  // Prio 1: OTA activity / failure.
  if (gFwUpdateInProgress) {
    bool alt = ((nowMs / 120) % 2) == 0;
    if (alt) ledWriteRaw(160, 0, 180); else ledWriteRaw(0, 150, 200);
    return;
  }
  if (gFwUpdateError.length()) {
    bool on = ((nowMs / 160) % 2) == 0;
    ledWriteRaw(on ? 220 : 0, 0, 0);
    return;
  }

  // Prio 2: network states.
  wl_status_t wifiSt = WiFi.status();
  wifi_mode_t wifiMode = WiFi.getMode();
  if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
    // Orange double-blink: 120ms ON, 120ms OFF, 120ms ON, 640ms OFF
    uint16_t p = (uint16_t)(nowMs % 1000);
    bool on = (p < 120) || (p >= 240 && p < 360);
    ledWriteRaw(on ? 180 : 0, on ? 70 : 0, 0);
    return;
  }
  if (wifiSt != WL_CONNECTED) {
    uint8_t p = ledPulse8(nowMs, 1000, 8, 100);
    ledWriteRaw(0, p, p);
    return;
  }

  // Prio 3: transport/turntable issues.
  if (!gBleConnected) {
    bool on = ((nowMs / 500) % 2) == 0;
    ledWriteRaw(on ? 180 : 0, 0, 0);
    return;
  }

  // Prio 4: sequencer states.
  if (gSeqState == SEQ_PAUSED) {
    uint8_t p = ledPulse8(nowMs, 900, 20, 120);
    ledWriteRaw(p, (uint8_t)(p * 0.55f), 0);
    return;
  }

  if (gSeqState == SEQ_RUNNING) {
    if (gLedShotFlashUntilMs > nowMs) {
      ledWriteRaw(160, 160, 160);
      return;
    }

    if (gSub == SUB_FLASH_COOLDOWN && gFlashGuardUntilMs > nowMs) {
      uint32_t remain = gFlashGuardUntilMs - nowMs;
      // Speed up pulse as guard cooldown approaches zero.
      uint16_t period = (remain > 60000) ? 900 : (remain > 15000 ? 500 : 260);
      uint8_t p = ledPulse8(nowMs, period, 18, 140);
      ledWriteRaw(p, (uint8_t)(p * 0.55f), 0);
      return;
    }

    if (gSub == SUB_RECOVER || gSub == SUB_RECOVER_WAIT || gSub == SUB_ROT_SEEK_WAIT) {
      bool alt = ((nowMs / 220) % 2) == 0;
      if (alt) ledWriteRaw(170, 45, 0); else ledWriteRaw(120, 0, 0);
      return;
    }

    if (gSub == SUB_TILT_SEND || gSub == SUB_TILT_WAIT) {
      uint8_t p = ledPulse8(nowMs, 1000, 12, 120);
      ledWriteRaw(p, (uint8_t)(p * 0.45f), 0);
      return;
    }

    if (gSub == SUB_ROT_SEND || gSub == SUB_ROT_WAIT) {
      uint8_t p = ledPulse8(nowMs, 450, 10, 130);
      ledWriteRaw(0, (uint8_t)(p * 0.35f), p);
      return;
    }

    if (gSub == SUB_SNAP) {
      ledWriteRaw(180, 180, 180);
      return;
    }

    // Progress color from blue -> green, plus subtle pulse.
    float frac = 0.0f;
    if (gTotalSteps > 0) frac = (float)gDoneSteps / (float)gTotalSteps;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    LedRgb start = {0, 50, 180};
    LedRgb end = {0, 170, 35};
    LedRgb c = ledBlend(start, end, frac);
    uint8_t pulse = ledPulse8(nowMs, 800, 160, 255);
    c.r = (uint8_t)(((uint16_t)c.r * pulse) / 255u);
    c.g = (uint8_t)(((uint16_t)c.g * pulse) / 255u);
    c.b = (uint8_t)(((uint16_t)c.b * pulse) / 255u);
    ledWriteRaw(c.r, c.g, c.b);
    return;
  }

  // IDLE victory phase.
  if (gLedVictoryUntilMs > nowMs) {
    uint8_t p = ledPulse8(nowMs, 300, 35, 190);
    ledWriteRaw(0, p, 0);
    return;
  }

  // Default ready state.
  ledWriteRaw(0, 35, 0);
}

/*
Turntable control reference (vendor template):
  +"COMMAND","ACTION"="VALUE";
Rules: ALL CAPS, no spaces, starts with '+', ends with ';'

Tilt:
  +CR,TILTVALUE=10;
  +CR,TILTSPEED=10;
  +CR,TOZERO;
  +CR,STOP;
  +QR,TILTANGLE;
  +QR,TILTSPEED;

Rotate:
  +CT,TURNANGLE=360;   // incremental, supports +/- values
  +CT,TURNSPEED=36;
  +CT,TOZERO;
  +CT,STOP;
  +QT,CHANGEANGLE;
  +QT,TURNSPEED;
*/
static float gManualTurnPulseDeg = 12.0f;
static uint32_t gManualTurnPulseMs = 180;
static int8_t gManualTurnDir = 0; // -1 left, +1 right, 0 stop
static uint32_t gManualTurnNextMs = 0;
static float gManualTiltStepDeg = 2.0f;
static uint32_t gManualTiltPulseMs = 180;
static int8_t gManualTiltDir = 0; // -1 down, +1 up, 0 stop
static float gManualTiltTarget = 0.0f;
static uint32_t gManualTiltNextMs = 0;

 
 
static float angDistDeg(float a, float b) {
  float d = fabsf(a - b);
  if (d > 180.0f) d = 360.0f - d;
  return d;
}
static float normAngle360(float a) {
  while (a < 0.0f) a += 360.0f;
  while (a >= 360.0f) a -= 360.0f;
  return a;
}
static float rotDeltaCw(float current, float target) {
  float c = normAngle360(current);
  float t = normAngle360(target);
  float d = t - c;
  if (d < 0.0f) d += 360.0f;
  return d;
}
static float lerp(float a, float b, float t) { return a + (b - a) * t; }
static float tiltAtIndex(int idx, int steps, float from, float to) {
  if (steps <= 1) return (from + to) * 0.5f;
  float t = (float)idx / (float)(steps - 1);
  return lerp(from, to, t);
}

static void seqResetInternal() {
  gSeqState = SEQ_IDLE;
  gSub = SUB_NONE;

  gTiltIdx = 0;
  gRotIdx = 0;

  if (gRotSteps < 1) gRotSteps = 1;
  if (gTiltSteps < 1) gTiltSteps = 1;
  if (gTiltFrom > gTiltTo) { float tmp = gTiltFrom; gTiltFrom = gTiltTo; gTiltTo = tmp; }

  gStepDeg = 360.0f / (float)gRotSteps;

  gTotalSteps = (uint32_t)gRotSteps * (uint32_t)gTiltSteps;
  gDoneSteps = 0;

  gCurTiltTarget = tiltAtIndex(0, gTiltSteps, gTiltFrom, gTiltTo);
  gRotTargetDeg = 0.0f;
  gRotRowZeroDeg = 0.0f;

  gStateTs = millis();
  gNextPollTs = 0;
  gResumePending = false;
  gResumeSub = SUB_NONE;
  gRecovering = false;
  gRecoverAttempts = 0;
  gRecoverStartMs = 0;
  gRecoverResendNextMs = 0;
  gNeedRecoverOnConnect = false;
  gRepeatStep = false;
  gHasResumeAngle = false;
  gSeekAttempts = 0;
  gFlashGuardUntilMs = 0;
  gNeedPhoneRecoverOnConnect = false;
  gSeqTriggerMode = gTriggerMode;

  gRotStepStartMs = 0;
  gRotStepMsAvg = 0.0f;
  gRotStepMsSamples = 0;
  gManualTurnDir = 0;
  gManualTurnNextMs = 0;
  gManualTiltDir = 0;
  gManualTiltNextMs = 0;
  gManualTiltTarget = gCurTiltTarget;
}

static void seqStart() {
  if (!gBleConnected) return;
  if (gTriggerMode == TRIGGER_MODE_SMARTPHONE && !gPhoneConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] START blocked: smartphone trigger selected but phone not connected\"}");
    return;
  }
  seqResetInternal();
  gSeqTriggerMode = gTriggerMode;
  gSeqState = SEQ_RUNNING;
  gSub = SUB_TILT_SEND;
  gResumePending = false;
  gResumeSub = SUB_NONE;
  gRecovering = false;
  gRecoverAttempts = 0;
  gRecoverStartMs = 0;
  gRecoverResendNextMs = 0;
  gNeedRecoverOnConnect = false;
  gRepeatStep = false;
  gHasResumeAngle = false;
  gSeekAttempts = 0;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] START\"}");
}
static void seqPause() { if (gSeqState == SEQ_RUNNING) { gSeqState = SEQ_PAUSED; wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] PAUSE\"}"); } }
static void seqResume(){
  if (gSeqState == SEQ_PAUSED)  {
    gSeqState = SEQ_RUNNING;
    if (gResumePending) {
      gSub = gResumeSub;
      gResumePending = false;
      gResumeSub = SUB_NONE;
    }
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] RESUME\"}");
  }
}
static void seqAbort() {
  if (gSeqState == SEQ_IDLE) return;
  seqResetInternal();
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] ABORT\"}");
}

static void seqPauseForBleLoss(bool repeatCurrentStep) {
  if (gSeqState != SEQ_RUNNING) return;
  gSeqState = SEQ_PAUSED;
  gResumePending = true;
  gResumeSub = SUB_RECOVER;
  gRecovering = true;
  gRecoverAttempts = 0;
  gRecoverStartMs = millis();
  gRecoverResendNextMs = 0;
  gNeedRecoverOnConnect = true;
  gNeedPhoneRecoverOnConnect = false;
  gRepeatStep = repeatCurrentStep;
  gHasResumeAngle = false;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] BLE lost -> PAUSE\"}");
}

static void seqPauseForPhoneLoss(bool repeatCurrentStep) {
  if (gSeqState != SEQ_RUNNING) return;
  gSeqState = SEQ_PAUSED;
  gResumePending = true;
  gResumeSub = SUB_RECOVER;
  gRecovering = true;
  gRecoverAttempts = 0;
  gRecoverStartMs = millis();
  gRecoverResendNextMs = 0;
  gNeedRecoverOnConnect = false;
  gNeedPhoneRecoverOnConnect = true;
  gRepeatStep = repeatCurrentStep;
  gHasResumeAngle = false;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] PHONE lost -> PAUSE\"}");
}

static void seqResumeRecoverAfterReconnect() {
  if (!gNeedRecoverOnConnect || gSeqState == SEQ_IDLE || !gBleConnected) return;
  gSeqState = SEQ_RUNNING;
  gSub = SUB_RECOVER;
  gResumePending = false;
  gResumeSub = SUB_NONE;
  gRecovering = true;
  gRecoverAttempts = 0;
  gRecoverStartMs = millis();
  gRecoverResendNextMs = 0;
  gStateTs = millis();
  gNextPollTs = 0;
  gAngleUpdated = false;
  gNeedRecoverOnConnect = false;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] BLE reconnected -> RECOVER\"}");
}

static void seqResumeRecoverAfterPhoneReconnect() {
  if (!gNeedPhoneRecoverOnConnect || gSeqState == SEQ_IDLE || !gPhoneConnected || !gBleConnected) return;
  gSeqState = SEQ_RUNNING;
  gSub = SUB_RECOVER;
  gResumePending = false;
  gResumeSub = SUB_NONE;
  gRecovering = true;
  gRecoverAttempts = 0;
  gRecoverStartMs = millis();
  gRecoverResendNextMs = 0;
  gStateTs = millis();
  gNextPollTs = 0;
  gAngleUpdated = false;
  gNeedPhoneRecoverOnConnect = false;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] PHONE reconnected -> RECOVER\"}");
}

static bool manualTurnStart(int8_t dir, const char* dirLabel) {
  if (!gBleConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual move ignored (BLE disconnected)\"}");
    return false;
  }
  if (gSeqState == SEQ_RUNNING) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual move ignored (sequence running)\"}");
    return false;
  }
  gManualTurnDir = (dir < 0) ? -1 : 1;
  gManualTurnNextMs = 0;
  bool ok = true;
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[TT] manual ")
                  + dirLabel + " START\"}");
  return ok;
}

static bool manualTurnStop() {
  if (!gBleConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual stop ignored (BLE disconnected)\"}");
    return false;
  }
  gManualTurnDir = 0;
  gManualTurnNextMs = 0;
  gManualTiltDir = 0;
  gManualTiltNextMs = 0;
  bool ok1 = bleWriteRaw("+CT,STOP;");
  bool ok2 = bleWriteRaw("+CR,STOP;");
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[TT] manual stop ")
                  + ((ok1 && ok2) ? "OK" : "FAIL") + "\"}");
  return ok1 && ok2;
}

static bool manualRotToZero() {
  if (!gBleConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] rot zero ignored (BLE disconnected)\"}");
    return false;
  }
  if (gSeqState == SEQ_RUNNING) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] rot zero ignored (sequence running)\"}");
    return false;
  }
  gManualTurnDir = 0;
  gManualTurnNextMs = 0;
  bool ok = bleWriteRaw("+CT,TOZERO;");
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[TT] rot zero ")
                  + (ok ? "OK" : "FAIL") + "\"}");
  return ok;
}

static bool manualTiltToZero() {
  if (!gBleConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] tilt zero ignored (BLE disconnected)\"}");
    return false;
  }
  if (gSeqState == SEQ_RUNNING) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] tilt zero ignored (sequence running)\"}");
    return false;
  }
  gManualTiltDir = 0;
  gManualTiltNextMs = 0;
  bool ok = bleWriteRaw("+CR,TOZERO;");
  if (ok) gCurTiltTarget = 0.0f;
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[TT] tilt zero ")
                  + (ok ? "OK" : "FAIL") + "\"}");
  return ok;
}

static void manualTurnTick() {
  if (gManualTurnDir == 0) return;
  if (!gBleConnected) { gManualTurnDir = 0; return; }
  if (gSeqState == SEQ_RUNNING) return;

  uint32_t now = millis();
  if ((int32_t)(now - gManualTurnNextMs) < 0) return;

  float delta = (gManualTurnDir < 0 ? -gManualTurnPulseDeg : gManualTurnPulseDeg);
  String cmd = String("+CT,TURNANGLE=") + String(delta, 1) + ";";
  bool ok = bleWriteRaw(cmd);
  if (!ok) {
    gManualTurnDir = 0;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual pulse FAIL -> stop\"}");
    return;
  }
  gManualTurnNextMs = now + gManualTurnPulseMs;
}

static bool manualTiltStart(int8_t dir, const char* dirLabel) {
  if (!gBleConnected) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual tilt ignored (BLE disconnected)\"}");
    return false;
  }
  if (gSeqState == SEQ_RUNNING) {
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual tilt ignored (sequence running)\"}");
    return false;
  }
  gManualTiltDir = (dir < 0) ? -1 : 1;
  gManualTiltNextMs = 0;
  gManualTiltTarget = gCurTiltTarget;
  wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[TT] manual tilt ")
                  + dirLabel + " START\"}");
  return true;
}

static void manualTiltTick() {
  if (gManualTiltDir == 0) return;
  if (!gBleConnected) { gManualTiltDir = 0; return; }
  if (gSeqState == SEQ_RUNNING) return;

  uint32_t now = millis();
  if ((int32_t)(now - gManualTiltNextMs) < 0) return;

  float minTilt = (gTiltFrom < gTiltTo) ? gTiltFrom : gTiltTo;
  float maxTilt = (gTiltFrom < gTiltTo) ? gTiltTo : gTiltFrom;
  gManualTiltTarget += (gManualTiltDir < 0 ? -gManualTiltStepDeg : gManualTiltStepDeg);
  if (gManualTiltTarget < minTilt) gManualTiltTarget = minTilt;
  if (gManualTiltTarget > maxTilt) gManualTiltTarget = maxTilt;

  String cmd = String("+CR,TILTVALUE=") + String(gManualTiltTarget, 1) + ";";
  bool ok = bleWriteRaw(cmd);
  if (!ok) {
    gManualTiltDir = 0;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[TT] manual tilt pulse FAIL -> stop\"}");
    return;
  }
  gCurTiltTarget = gManualTiltTarget;
  gManualTiltNextMs = now + gManualTiltPulseMs;
}

static void seqTick() {
  if (gSeqState != SEQ_RUNNING) return;
  if (!gBleConnected) {
    seqPauseForBleLoss(false);
    return;
  }

  uint32_t now = millis();

  switch (gSub) {
    case SUB_RECOVER: {
      // Stop any ongoing motion after power loss / reconnect
      bleWriteRaw("+CT,STOP;");
      bleWriteRaw("+CR,STOP;");
      gCurTiltTarget = tiltAtIndex(gTiltIdx, gTiltSteps, gTiltFrom, gTiltTo);
      String cmd = String("+CR,TILTVALUE=") + String(gCurTiltTarget, 1) + ";";
      bleWriteRaw(cmd);
      {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "[SEQ] RECOVER tilt %d/%d -> target=%.1f deg",
                 gTiltIdx + 1, gTiltSteps, gCurTiltTarget);
        String j = String("{\"type\":\"log\",\"msg\":\"") + jsonEscape(buf) + "\"}";
        wsBroadcastJson(j);
      }
      gStateTs = now;
      gNextPollTs = now;
      gAngleUpdated = false;
      gRecoverLastAngle = gLastAngle;
      gRecoverStableSince = 0;
      gRecovering = true;
      if (gRecoverStartMs == 0) gRecoverStartMs = now;
      if (gRecoverAttempts < 255) gRecoverAttempts++;
      gRecoverResendNextMs = now + 1000;
      gSub = SUB_RECOVER_WAIT;
    } break;

    case SUB_RECOVER_WAIT: {
      uint32_t waitMs = gTiltMoveMs + gTiltReserveMs;
      // If we are not receiving any angle updates, force a reconnect and retry recovery
      if ((now - gLastRxMs) > 3000) {
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] RECOVER no RX -> BLE reconnect\"}");
        bleDisconnect();
        gSeqState = SEQ_PAUSED;
        gResumePending = true;
        gResumeSub = SUB_RECOVER;
        return;
      }
      // Re-issue STOP and TILT periodically during recovery
      if ((int32_t)(now - gRecoverResendNextMs) >= 0) {
        bleWriteRaw("+CT,STOP;");
        bleWriteRaw("+CR,STOP;");
        String cmd = String("+CR,TILTVALUE=") + String(gCurTiltTarget, 1) + ";";
        bleWriteRaw(cmd);
        gRecoverResendNextMs = now + 1000;
      }
      if ((int32_t)(now - gNextPollTs) >= 0) {
        bleWriteRaw("+QT,CHANGEANGLE;");
        gNextPollTs = now + gPollMs;
      }
      if (gAngleUpdated) {
        gAngleUpdated = false;
        float dist = angDistDeg(gLastAngle, gRecoverLastAngle);
        gRecoverLastAngle = gLastAngle;
        if (dist <= gRotTolDeg) {
          if (gRecoverStableSince == 0) gRecoverStableSince = now;
        } else {
          gRecoverStableSince = 0;
          // still moving -> re-issue stop to suppress auto-rotate
          bleWriteRaw("+CT,STOP;");
          bleWriteRaw("+CR,STOP;");
        }
      }
      bool tiltDone = (now - gStateTs >= waitMs);
      bool stable = (gRecoverStableSince != 0) && (now - gRecoverStableSince >= (2 * gPollMs));
      if (tiltDone && (stable || (now - gStateTs > (waitMs + 8000)))) {
        if (gRepeatStep) {
          if (gRotIdx > 0) gRotIdx--;
          if (gDoneSteps > 0) gDoneSteps--;
          gRepeatStep = false;
        }
        gHasResumeAngle = false;
        gSeekAttempts = 0;
        gSub = SUB_ROT_SEND;
      }
    } break;

    case SUB_ROT_SEEK_WAIT: {
      if ((int32_t)(now - gNextPollTs) >= 0) {
        bleWriteRaw("+QT,CHANGEANGLE;");
        gNextPollTs = now + gPollMs;
      }
      if ((now - gLastRxMs) > 3000) {
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] SEEK no RX -> BLE reconnect\"}");
        bleDisconnect();
        gSeqState = SEQ_PAUSED;
        gResumePending = true;
        gResumeSub = SUB_RECOVER;
        return;
      }
      if (gAngleUpdated) {
        gAngleUpdated = false;
        float dist = angDistDeg(gLastAngle, gRotTargetDeg);
        if (dist <= gRotTolDeg) {
          gSub = SUB_ROT_SEND;
        }
      }
      if (now - gStateTs > (gRotTimeoutMs + 8000)) {
        if (gSeekAttempts >= 2) {
          wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] SEEK TIMEOUT -> SKIP SEEK\"}");
          gSeekAttempts = 0;
          gSub = SUB_ROT_SEND;
        } else {
          wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] SEEK TIMEOUT -> RECOVER\"}");
          gSub = SUB_RECOVER;
        }
        gHasResumeAngle = false;
      }
    } break;

    case SUB_TILT_SEND: {
      gCurTiltTarget = tiltAtIndex(gTiltIdx, gTiltSteps, gTiltFrom, gTiltTo);
      String cmd = String("+CR,TILTVALUE=") + String(gCurTiltTarget, 1) + ";";
      bleWriteRaw(cmd);
      {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "[SEQ] TILT step %d/%d -> target=%.1f deg",
                 gTiltIdx + 1, gTiltSteps, gCurTiltTarget);
        String j = String("{\"type\":\"log\",\"msg\":\"") + jsonEscape(buf) + "\"}";
        wsBroadcastJson(j);
      }
      gStateTs = now;
      gSub = SUB_TILT_WAIT;
    } break;

    case SUB_TILT_WAIT: {
      uint32_t waitMs = gTiltMoveMs + gTiltReserveMs;
      if (now - gStateTs >= waitMs) {
        gRotIdx = 0;
        gSub = SUB_ROT_SEND;
      }
    } break;

    case SUB_ROT_SEND: {
      float start = gLastAngle;
      gRotTargetDeg = normAngle360(start + gStepDeg);

      gAngleUpdated = false;
      gNextPollTs = now;
      gStateTs = now;
      gRotStepStartMs = now;

      String cmd = String("+CT,TURNANGLE=") + String(gStepDeg, 1) + ";";
      bleWriteRaw(cmd);
      {
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "[SEQ] ROT step %d/%d (tilt %d/%d) -> target=%.2f deg (start=%.2f, step=%.2f)",
                 gRotIdx + 1, gRotSteps, gTiltIdx + 1, gTiltSteps,
                 gRotTargetDeg, start, gStepDeg);
        String j = String("{\"type\":\"log\",\"msg\":\"") + jsonEscape(buf) + "\"}";
        wsBroadcastJson(j);
      }

      gSub = SUB_ROT_WAIT;
    } break;

    case SUB_ROT_WAIT: {
      if ((int32_t)(now - gNextPollTs) >= 0) {
        bleWriteRaw("+QT,CHANGEANGLE;");
        gNextPollTs = now + gPollMs;
      }
      // If we are not receiving any angle updates, force a reconnect and retry recovery
      if ((now - gLastRxMs) > 3000) {
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] ROT no RX -> BLE reconnect\"}");
        bleDisconnect();
        gSeqState = SEQ_PAUSED;
        gResumePending = true;
        gResumeSub = SUB_RECOVER;
        return;
      }

      if (gAngleUpdated) {
        gAngleUpdated = false;
        float dist = angDistDeg(gLastAngle, gRotTargetDeg);
        if (dist <= gRotTolDeg) {
          if (!gRecovering && gRotStepStartMs != 0) {
            uint32_t dur = now - gRotStepStartMs;
            if (gRotStepMsSamples == 0) gRotStepMsAvg = (float)dur;
            else gRotStepMsAvg = (gRotStepMsAvg * 0.8f) + ((float)dur * 0.2f);
            gRotStepMsSamples++;
          }
          gSub = SUB_SETTLE;
          gStateTs = now;
          gRecovering = false;
          gRecoverAttempts = 0;
        }
      }

      uint32_t rotTimeout = gRotTimeoutMs + (gRecovering ? 12000 : 0);
      if (now - gStateTs > rotTimeout) {
        bool recoverWindowOk = (gRecoverStartMs == 0) || (now - gRecoverStartMs < 120000);
        if (gRecovering && recoverWindowOk) {
          wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] ROT TIMEOUT -> RECOVER\"}");
          gSub = SUB_RECOVER;
          gStateTs = now;
          gNextPollTs = now;
          gAngleUpdated = false;
        } else {
          char buf[128];
          snprintf(buf, sizeof(buf),
                   "[SEQ] ROT TIMEOUT -> ABORT (angle=%.2f target=%.2f age=%lu ms)",
                   gLastAngle, gRotTargetDeg, (unsigned long)(now - gLastRxMs));
          String j = String("{\"type\":\"log\",\"msg\":\"") + jsonEscape(buf) + "\"}";
          wsBroadcastJson(j);
          seqAbort();
        }
      }
    } break;

    case SUB_SETTLE: {
      if (now - gStateTs >= gSnapSettleMs) {
        gSub = SUB_SNAP;
      }
    } break;

    case SUB_SNAP: {
      wsBroadcastJson("{\"type\":\"rigLine\",\"line\":\"SNAP\"}");
      bool ok = triggerCamera();
      wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] ")
                      + (ok ? "OK" : "FAIL") + "\"}");

      gStateTs = now;
      gSub = SUB_COOLDOWN;
    } break;

    case SUB_COOLDOWN: {
      if (now - gStateTs >= gSnapCooldownMs) {
        gDoneSteps++;
        bool useFlashGuard =
          gFlashGuardEnabled &&
          (gTriggerMode == TRIGGER_MODE_HW) &&
          (gFlashGuardEveryShots > 0) &&
          (gFlashGuardMs > 0) &&
          (gDoneSteps < gTotalSteps) &&
          ((gDoneSteps % gFlashGuardEveryShots) == 0);

        if (useFlashGuard) {
          gFlashGuardUntilMs = now + gFlashGuardMs;
          gSub = SUB_FLASH_COOLDOWN;
          wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SEQ] FLASH COOLDOWN start (")
                          + String(gFlashGuardMs / 1000) + " s)\"}");
        } else {
          seqAdvanceAfterShot();
        }
      }
    } break;

    case SUB_FLASH_COOLDOWN: {
      if ((int32_t)(now - gFlashGuardUntilMs) >= 0) {
        gFlashGuardUntilMs = 0;
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] FLASH COOLDOWN done\"}");
        seqAdvanceAfterShot();
      }
    } break;

    case SUB_DONE: {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] DONE\"}");
      gSeqState = SEQ_IDLE;
      gSub = SUB_NONE;
    } break;

    default: break;
  }
}

// ===== Rig state JSON builder =====
static String buildRigStateJson() {
  String state = (gSeqState == SEQ_IDLE) ? "IDLE" : (gSeqState == SEQ_RUNNING ? "RUNNING" : "PAUSED");
  bool bleNow = gBleConnected;
  uint32_t nowMs = millis();


  // STEP formatted as "cur/total"
  String stepStr = String((unsigned long)gDoneSteps) + "/" + String((unsigned long)gTotalSteps);

  String ip = (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

  // Build a compact object with keys used by UI
  String j = "{";
  j += "\"STATE\":\"" + state + "\",";
  j += "\"STEP\":\"" + stepStr + "\",";
  j += "\"ROT_STEPS\":\"" + String(gRotSteps) + "\",";
  j += "\"TILT_STEPS\":\"" + String(gTiltSteps) + "\",";
  j += "\"TILT_FROM\":\"" + String(gTiltFrom, 1) + "\",";
  j += "\"TILT_TO\":\"" + String(gTiltTo, 1) + "\",";
  j += "\"TILT_MOVE_MS\":\"" + String(gTiltMoveMs) + "\",";
  j += "\"TILT_RESERVE_MS\":\"" + String(gTiltReserveMs) + "\",";
  j += "\"SNAP_SETTLE_MS\":\"" + String(gSnapSettleMs) + "\",";
  j += "\"SNAP_COOLDOWN_MS\":\"" + String(gSnapCooldownMs) + "\",";
  j += "\"ROT_STEP_MS_AVG\":\"" + String(gRotStepMsAvg, 1) + "\",";
  j += "\"ANGLE_LAST\":\"" + String(gLastAngle, 2) + "\",";
  j += "\"FW_VER\":\"" + String(FW_VERSION) + "\",";
  j += "\"UI_VER\":\"" + String(UI_VERSION) + "\",";
  j += "\"BUILD_GIT\":\"" + jsonEscape(String(BUILD_GIT)) + "\",";
  j += "\"BUILD_TIME\":\"" + jsonEscape(String(BUILD_TIME)) + "\",";
  j += "\"UPDATE_URL\":\"" + jsonEscape(String(UPDATE_MANIFEST_URL)) + "\",";
  j += "\"TRIGGER_MODE\":\"" + String(gTriggerMode == TRIGGER_MODE_SMARTPHONE ? "SMARTPHONE" : "HARDWARE") + "\",";
  j += "\"TRIGGER_ENABLED\":\"" + String(gTriggerEnabled ? 1 : 0) + "\",";
  j += "\"AF_MODE\":\"" + String(gAutoFocusEnabled ? "AUTO" : "MANUAL") + "\",";
  j += "\"AF_PREFOCUS_MS\":\"" + String(CAM_AF_PREFOCUS_MS) + "\",";
  j += "\"AF_SHUTTER_MS\":\"" + String(CAM_AF_SHUTTER_MS) + "\",";
  j += "\"AF_POSTFOCUS_MS\":\"" + String(CAM_AF_POSTFOCUS_MS) + "\",";
  j += "\"SNAP_PRESS_MS\":\"" + String(CAM_PRESS_MS) + "\",";
  j += "\"MANUAL_PREFOCUS_MS\":\"" + String(CAM_PREFOCUS_MS) + "\",";
  j += "\"FLASH_GUARD\":\"" + String(gFlashGuardEnabled ? 1 : 0) + "\",";
  j += "\"FLASH_GUARD_EVERY\":\"" + String(gFlashGuardEveryShots) + "\",";
  j += "\"FLASH_GUARD_MS\":\"" + String(gFlashGuardMs) + "\",";
  uint32_t flashRemainMs = 0;
  if (gSeqState == SEQ_RUNNING && gSub == SUB_FLASH_COOLDOWN && gFlashGuardUntilMs > nowMs) {
    flashRemainMs = gFlashGuardUntilMs - nowMs;
  }
  j += "\"FLASH_GUARD_REMAIN_MS\":\"" + String(flashRemainMs) + "\",";
  j += "\"PHONE_BT\":\"" + String(gPhoneConnected ? 1 : 0) + "\",";
  j += "\"PHONE_PAIRING\":\"" + String(gPhonePairing ? 1 : 0) + "\",";
  j += "\"PHONE_NAME\":\"" + jsonEscape(String(SMARTPHONE_BT_NAME)) + "\",";
  j += "\"PHONE_PEER\":\"" + jsonEscape(gPhonePeerName) + "\",";
  j += "\"BLE\":\"" + String(bleNow ? 1 : 0) + "\",";
  j += "\"TT\":\"" + String(bleNow ? 1 : 0) + "\",";
  j += "\"IP\":\"" + ip + "\",";
  j += "\"HOST\":\"" + jsonEscape(String(MDNS_NAME) + ".local") + "\",";
  j += "\"WIFI_MODE\":\"" + jsonEscape(wifiModeText()) + "\",";
  j += "\"PORT\":\"" + String(HTTP_PORT) + "\",";
  j += "\"SUB\":\"" + String((int)gSub) + "\"";
  j += "}";
  return j;
}

static void wsSendHello(AsyncWebSocketClient* client) {
  String rig = buildRigStateJson();
  String msg = String("{\"type\":\"hello\",\"rigState\":") + rig + ",\"tcpConnected\":true}";
  client->text(msg);
}

static void wsSendStatus() {
  String rig = buildRigStateJson();
  String msg = String("{\"type\":\"status\",\"rigState\":") + rig + ",\"tcpConnected\":true}";
  ws.textAll(msg);
}

// ===== Command parsing =====
static void setKeyVal(const String& key, const String& val) {
  if (key == "ROT_STEPS") {
    gRotSteps = val.toInt();
    seqResetInternal();
  } else if (key == "TILT_STEPS") {
    gTiltSteps = val.toInt();
    seqResetInternal();
  } else if (key == "TILT_FROM") {
    gTiltFrom = val.toFloat();
    seqResetInternal();
  } else if (key == "TILT_TO") {
    gTiltTo = val.toFloat();
    seqResetInternal();
  } else if (key == "POLL_MS") {
    gPollMs = (uint32_t)val.toInt();
  } else if (key == "ROT_TIMEOUT_MS") {
    gRotTimeoutMs = (uint32_t)val.toInt();
  } else if (key == "ROT_TOL_DEG") {
    gRotTolDeg = val.toFloat();
  } else if (key == "TILT_MOVE_MS") {
    gTiltMoveMs = (uint32_t)val.toInt();
  } else if (key == "SNAP_SETTLE_MS") {
    gSnapSettleMs = (uint32_t)val.toInt();
  } else if (key == "SNAP_COOLDOWN_MS") {
    gSnapCooldownMs = (uint32_t)val.toInt();
  } else if (key == "TRIGGER_MODE") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger mode change blocked (sequence running)\"}");
      return;
    }
    String mode = val;
    mode.trim();
    mode.toUpperCase();
    TriggerMode nextMode = (mode == "SMARTPHONE") ? TRIGGER_MODE_SMARTPHONE : TRIGGER_MODE_HW;
    if (nextMode != gTriggerMode) {
      gTriggerMode = nextMode;
      saveTriggerMode(gTriggerMode);
      if (gTriggerMode == TRIGGER_MODE_HW) {
        phoneDisableForHardwareMode();
      }
      wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] mode -> ")
                      + (gTriggerMode == TRIGGER_MODE_SMARTPHONE ? "SMARTPHONE" : "HARDWARE") + "\"}");
    }
  } else if (key == "TRIGGER_ENABLED") {
    String on = val;
    on.trim();
    on.toUpperCase();
    bool nextEnabled = !(on == "0" || on == "FALSE" || on == "OFF" || on == "NO");
    if (nextEnabled != gTriggerEnabled) {
      gTriggerEnabled = nextEnabled;
      wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] trigger ")
                      + (gTriggerEnabled ? "ENABLED" : "DISABLED (dry-run)") + "\"}");
    }
  } else if (key == "AF_MODE") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode change blocked (sequence running)\"}");
      return;
    }
    String mode = val;
    mode.trim();
    mode.toUpperCase();
    bool nextAutoFocus = !(mode == "MANUAL" || mode == "OFF" || mode == "0");
    if (nextAutoFocus != gAutoFocusEnabled) {
      gAutoFocusEnabled = nextAutoFocus;
      saveAutoFocusMode(gAutoFocusEnabled);
      wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode -> ")
                      + (gAutoFocusEnabled ? "AUTO" : "MANUAL") + "\"}");
    }
  } else if (key == "FLASH_GUARD") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard change blocked (sequence running)\"}");
      return;
    }
    String on = val;
    on.trim();
    on.toUpperCase();
    bool nextEnabled = !(on == "0" || on == "FALSE" || on == "OFF" || on == "NO");
    if (nextEnabled != gFlashGuardEnabled) {
      gFlashGuardEnabled = nextEnabled;
      wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard ")
                      + (gFlashGuardEnabled ? "ENABLED" : "DISABLED") + "\"}");
    }
  } else if (key == "FLASH_GUARD_EVERY") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard change blocked (sequence running)\"}");
      return;
    }
    uint32_t v = (uint32_t)val.toInt();
    if (v < 1) v = 1;
    gFlashGuardEveryShots = v;
  } else if (key == "FLASH_GUARD_MS") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard change blocked (sequence running)\"}");
      return;
    }
    uint32_t v = (uint32_t)val.toInt();
    if (v < 1000) v = 1000;
    gFlashGuardMs = v;
  } else if (key == "WIFI_SSID") {
    wifiSsid = val;
    saveWifiCreds(wifiSsid, wifiPass);
  } else if (key == "WIFI_PASS") {
    wifiPass = val;
    saveWifiCreds(wifiSsid, wifiPass);
  }
}

static void handleLine(const String& lineIn) {
  String line = lineIn;
  line.trim();
  if (!line.length()) return;

  // Basic commands (UI sends uppercase)
  if (line == "STATUS") { wsSendStatus(); return; }
  if (line == "START")  { seqStart(); wsSendStatus(); return; }
  if (line == "PAUSE")  { seqPause(); wsSendStatus(); return; }
  if (line == "RESUME") { seqResume(); wsSendStatus(); return; }
  if (line == "ABORT")  { seqAbort(); wsSendStatus(); return; }
  if (line == "RESET")  { seqResetInternal(); wsSendStatus(); return; }
  if (line == "TT_LEFT")  { manualTurnStart(-1, "left"); wsSendStatus(); return; }
  if (line == "TT_RIGHT") { manualTurnStart(+1, "right"); wsSendStatus(); return; }
  if (line == "TT_ROT_ZERO") { manualRotToZero(); wsSendStatus(); return; }
  if (line == "TT_TILT_UP") { manualTiltStart(+1, "up"); wsSendStatus(); return; }
  if (line == "TT_TILT_DOWN") { manualTiltStart(-1, "down"); wsSendStatus(); return; }
  if (line == "TT_TILT_ZERO") { manualTiltToZero(); wsSendStatus(); return; }
  if (line == "TT_STOP" || line == "TT_PAUSE") { manualTurnStop(); wsSendStatus(); return; }
  if (line == "SNAP") {
    wsBroadcastJson("{\"type\":\"rigLine\",\"line\":\"SNAP\"}");
    bool ok = triggerCamera();
    wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] ")
                    + (ok ? "OK" : "FAIL") + "\"}");
    wsSendStatus();
    return;
  }
  if (line == "SNAP_FOCUS") {
    wsBroadcastJson("{\"type\":\"rigLine\",\"line\":\"SNAP_FOCUS\"}");
    bool ok = triggerCameraHardwareFocusOnly();
    wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] focus ")
                    + (ok ? "OK" : "FAIL") + "\"}");
    wsSendStatus();
    return;
  }
  if (line == "SNAP_TRIGGER") {
    wsBroadcastJson("{\"type\":\"rigLine\",\"line\":\"SNAP_TRIGGER\"}");
    bool ok = triggerCameraHardwareShutterOnly();
    wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[SNAP] trigger ")
                    + (ok ? "OK" : "FAIL") + "\"}");
    wsSendStatus();
    return;
  }
  if (line == "TRIGGER_ON") {
    gTriggerEnabled = true;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger ENABLED\"}");
    wsSendStatus();
    return;
  }
  if (line == "TRIGGER_OFF") {
    gTriggerEnabled = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger DISABLED (dry-run)\"}");
    wsSendStatus();
    return;
  }
  if (line == "AF_MODE_AUTO") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gAutoFocusEnabled = true;
    saveAutoFocusMode(gAutoFocusEnabled);
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode -> AUTO\"}");
    wsSendStatus();
    return;
  }
  if (line == "AF_MODE_MANUAL") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gAutoFocusEnabled = false;
    saveAutoFocusMode(gAutoFocusEnabled);
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] autofocus mode -> MANUAL\"}");
    wsSendStatus();
    return;
  }
  if (line == "PHONE_PAIR_START") {
    gReqPhonePairStart = true;
    gReqPhonePairStop = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[PHONE] pairing start requested\"}");
    wsSendStatus();
    return;
  }
  if (line == "PHONE_PAIR_STOP") {
    gReqPhonePairStop = true;
    gReqPhonePairStart = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[PHONE] pairing stop requested\"}");
    wsSendStatus();
    return;
  }
  if (line == "PHONE_DISCONNECT") {
    phoneDisconnect();
    wsSendStatus();
    return;
  }
  if (line == "FLASH_GUARD_ON") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gFlashGuardEnabled = true;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard ENABLED\"}");
    wsSendStatus();
    return;
  }
  if (line == "FLASH_GUARD_OFF") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gFlashGuardEnabled = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] flash guard DISABLED\"}");
    wsSendStatus();
    return;
  }
  if (line == "TRIGGER_MODE_HW") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger mode change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gTriggerMode = TRIGGER_MODE_HW;
    saveTriggerMode(gTriggerMode);
    phoneDisableForHardwareMode();
    wsSendStatus();
    return;
  }
  if (line == "TRIGGER_MODE_SMARTPHONE") {
    if (gSeqState == SEQ_RUNNING) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SNAP] trigger mode change blocked (sequence running)\"}");
      wsSendStatus();
      return;
    }
    gTriggerMode = TRIGGER_MODE_SMARTPHONE;
    saveTriggerMode(gTriggerMode);
    wsSendStatus();
    return;
  }

  // BLE control
  if (line == "BLE_CONNECT") { bool ok = bleConnect(); wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[BLE] connect ") + (ok ? "OK" : "FAIL") + "\"}"); wsSendStatus(); return; }
  if (line == "BLE_DISCONNECT") { bleDisconnect(); wsSendStatus(); return; }

  // RAW passthrough: "RAW +QT,CHANGEANGLE;"
  if (line.startsWith("RAW ")) {
    String raw = line.substring(4);
    raw.trim();
    if (raw.length()) bleWriteRaw(raw);
    return;
  }

  // SET KEY=VAL
  if (line.startsWith("SET ")) {
    String rest = line.substring(4);
    rest.trim();
    int eq = rest.indexOf('=');
    if (eq > 0) {
      String key = rest.substring(0, eq); key.trim();
      String val = rest.substring(eq + 1); val.trim();
      setKeyVal(key, val);
      wsSendStatus();
      return;
    }
  }

  // Unknown -> log
  String j = String("{\"type\":\"log\",\"msg\":\"[CMD] unknown: ") + jsonEscape(line) + "\"}";
  wsBroadcastJson(j);
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
  (void)server; (void)arg;

  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] client #%u connected\n", client->id());
    wsSendHello(client);
    // also broadcast a status snapshot so UI updates immediately
    wsSendStatus();
    return;
  }

  if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] client #%u disconnected\n", client->id());
    return;
  }

  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (!info->final || info->index != 0 || info->len != len) return;
    if (info->opcode != WS_TEXT) return;

    String s;
    s.reserve(len + 1);
    for (size_t i = 0; i < len; i++) s += (char)data[i];

    // Very small JSON parsing (only what we need)
    // Expect: {"type":"requestStatus"} or {"type":"cmd","line":"..."}
    if (s.indexOf("\"type\"") < 0) return;

    if (s.indexOf("requestStatus") >= 0) {
      wsSendHello(client);
      return;
    }

    if (s.indexOf("\"type\":\"cmd\"") >= 0 || s.indexOf("\"type\": \"cmd\"") >= 0) {
      int p = s.indexOf("\"line\"");
      if (p < 0) return;
      p = s.indexOf(':', p);
      if (p < 0) return;

      // Find first quote after colon
      int q1 = s.indexOf('\"', p);
      if (q1 < 0) return;
      int q2 = s.indexOf('\"', q1 + 1);
      if (q2 < 0) return;

      String line = s.substring(q1 + 1, q2);
      line.replace("\\n", "\n");
      line.replace("\\r", "\r");
      line.replace("\\\"", "\"");
      line.replace("\\\\", "\\");

      handleLine(line);
      return;
    }
  }
}

// ===== Serial CLI (WiFi setup helper) =====
static String readLineSerial() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String out = line;
      line = "";
      out.trim();
      return out;
    }
    line += c;
  }
  return "";
}

static void printHelpSerial() {
  Serial.println();
  Serial.println("=== ScanRig ESP32 UI Bridge ===");
  Serial.println("Serial commands:");
  Serial.println("  WIFI <ssid> <pass>     (save creds and reboot manually)");
  Serial.println("  BLE_CONNECT            (connect to turntable)");
  Serial.println("  BLE_DISCONNECT");
  Serial.println("  STATUS | START | PAUSE | RESUME | ABORT | RESET");
  Serial.println("  TT_LEFT | TT_RIGHT | TT_ROT_ZERO | TT_TILT_UP | TT_TILT_DOWN | TT_TILT_ZERO | TT_STOP");
  Serial.println("  SNAP | SNAP_FOCUS | SNAP_TRIGGER");
  Serial.println("  TRIGGER_ON | TRIGGER_OFF");
  Serial.println("  AF_MODE_AUTO | AF_MODE_MANUAL");
  Serial.println("  FLASH_GUARD_ON | FLASH_GUARD_OFF");
  Serial.println("  TRIGGER_MODE_HW | TRIGGER_MODE_SMARTPHONE");
  Serial.println("  PHONE_PAIR_START | PHONE_PAIR_STOP | PHONE_DISCONNECT");
  Serial.println("  SET KEY=VAL            e.g. SET ROT_STEPS=72");
  Serial.println("  RAW <raw>              e.g. RAW +QT,CHANGEANGLE;");
  Serial.println();
  Serial.println("WiFi:");
  Serial.printf("  STA ssid='%s'\n", wifiSsid.c_str());
  Serial.printf("  If STA fails, AP '%s' pass '%s'\n", AP_SSID, AP_PASS);
  Serial.printf("mDNS: http://%s.local/\n", MDNS_NAME);
}

static void handleSerialCmd(const String& cmd) {
  if (!cmd.length()) return;

  if (cmd == "HELP") { printHelpSerial(); return; }

  
if (cmd == "WIFI_FIRM") {
  wifiSsid = FIRM_SSID;
  wifiPass = FIRM_PASS;
  wifiUseStatic = (SCANRIG_FIRM_USE_STATIC != 0);
  wifiIpStr = FIRM_IP;
  wifiGwStr = FIRM_GW;
  wifiDnsStr = FIRM_DNS;
  wifiMaskStr = FIRM_MASK;
  saveWifiCreds(wifiSsid, wifiPass);
  saveWifiStatic(wifiUseStatic, wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr);
  Serial.println("[WIFI] saved FIRM profile. reboot ESP32 to apply.");
  return;
}

if (cmd == "WIFI_DHCP") {
  wifiUseStatic = false;
  saveWifiStatic(false, "", "", "", "");
  Serial.println("[WIFI] DHCP enabled (static disabled). reboot ESP32 to apply.");
  return;
}

if (cmd.startsWith("WIFI_STATIC ")) {
  // WIFI_STATIC <ip> <gw> <dns> <mask>
  String rest = cmd.substring(12);
  rest.trim();
  String parts[4];
  int pi = 0;
  int last = 0;
  for (int i = 0; i <= (int)rest.length(); i++) {
    if (i == (int)rest.length() || rest[i] == ' ') {
      String p = rest.substring(last, i);
      p.trim();
      if (p.length() && pi < 4) parts[pi++] = p;
      last = i + 1;
    }
  }
  if (pi != 4) {
    Serial.println("usage: WIFI_STATIC <ip> <gw> <dns> <mask>");
    return;
  }
  wifiUseStatic = true;
  wifiIpStr = parts[0];
  wifiGwStr = parts[1];
  wifiDnsStr = parts[2];
  wifiMaskStr = parts[3];
  saveWifiStatic(true, wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr);
  Serial.println("[WIFI] static IP saved. reboot ESP32 to apply.");
  return;
}

if (cmd.startsWith("WIFI ")) {
    // WIFI <ssid> <pass>
    String rest = cmd.substring(5);
    rest.trim();
    int sp = rest.indexOf(' ');
    if (sp < 0) {
      Serial.println("usage: WIFI <ssid> <pass>");
      return;
    }
    wifiSsid = rest.substring(0, sp);
    wifiPass = rest.substring(sp + 1);
    wifiSsid.trim(); wifiPass.trim();
    saveWifiCreds(wifiSsid, wifiPass);
    Serial.println("[WIFI] saved creds. reboot ESP32 to apply.");
    return;
  }

  // Mirror the same handler as UI commands.
  handleLine(cmd);
}

// ===== BLE Auto-connect / Reconnect =====
static uint32_t gBleNextAttemptMs = 0;
static uint32_t gBleBackoffMs = 2000;

static void bleNoteAttempt(bool ok) {
  if (ok) {
    gBleBackoffMs = 2000;
    gBleNextAttemptMs = 0;
  } else {
    if (gBleBackoffMs < 30000) gBleBackoffMs = min<uint32_t>(30000, gBleBackoffMs * 2);
    gBleNextAttemptMs = millis() + gBleBackoffMs;
  }
}

static void bleAutoConnectTick() {
  if (!gBleAutoConnect) return;
  if (gPhonePairing && !gPhoneConnected) return;

  if (gClient && !gClient->isConnected() && gBleConnected) {
    gBleConnected = false;
    gChr = nullptr;
  }
  if (gBleConnected) return;

  uint32_t now = millis();
  if (gBleNextAttemptMs && (int32_t)(now - gBleNextAttemptMs) < 0) return;

  Serial.println("[BLE] autoconnect attempt...");
  bool ok = bleConnect();
  const char* r = ok ? "OK" : "FAIL";
  Serial.printf("[BLE] autoconnect %s (backoff=%lu ms)\n", r, (unsigned long)gBleBackoffMs);
  bleNoteAttempt(ok);
}

static void bleHeartbeatTick();

static void bleHeartbeatTick() {
  if (!gClient) return;

  const uint32_t HEARTBEAT_INTERVAL_MS = 1500;
  const uint32_t HEARTBEAT_TIMEOUT_MS  = 4000;

  uint32_t now = millis();

  if (gClient->isConnected()) {
    if (gLastRxMs == 0) gLastRxMs = now;

    if ((now - gLastRxMs) > HEARTBEAT_TIMEOUT_MS) {
      Serial.println("[BLE] heartbeat timeout -> forcing disconnect");
      gClient->disconnect();
      gBleConnected = false;
      gChr = nullptr;
      gHeartbeatPending = false;
      return;
    }

    if ((now - gLastHeartbeatMs) >= HEARTBEAT_INTERVAL_MS) {
      gLastHeartbeatMs = now;
      bool ok = bleWriteRaw("+QT,CHANGEANGLE;");
      if (!ok) {
        Serial.println("[BLE] heartbeat write failed -> forcing disconnect");
        gClient->disconnect();
        gBleConnected = false;
        gChr = nullptr;
        gHeartbeatPending = false;
        return;
      }
      gHeartbeatPending = true;
    }
  } else {
    gHeartbeatPending = false;
    gLastRxMs = 0;
  }
}

// ===== Setup =====

void setup() {
  Serial.begin(115200);
  delay(200);
  ledWriteRaw(0, 0, 0);

  if (CAM_FOCUS_PIN >= 0) {
    pinMode(CAM_FOCUS_PIN, OUTPUT);
    digitalWrite(CAM_FOCUS_PIN, camIdleLevel());
  }
  if (CAM_SHUTTER_PIN >= 0) {
    pinMode(CAM_SHUTTER_PIN, OUTPUT);
    digitalWrite(CAM_SHUTTER_PIN, camIdleLevel());
  }

  loadTriggerMode();
  loadAutoFocusMode();
  loadStatusLedConfig();

  loadWifiCreds();
  wifiStart();
  mdnsStart();

  // BLE init (lazy connect)
  bleInitOnce();
  initSmartphoneTriggerBle();

  // Web routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html; charset=utf-8", INDEX_HTML, INDEX_HTML_LEN);
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html; charset=utf-8", INDEX_HTML, INDEX_HTML_LEN);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/css; charset=utf-8", STYLE_CSS, STYLE_CSS_LEN);
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/javascript; charset=utf-8", APP_JS, APP_JS_LEN);
  });
  server.on("/api/network", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json; charset=utf-8", buildNetworkConfigJson());
  });
  server.on("/api/network", HTTP_POST, [](AsyncWebServerRequest* req) {
    bool hasSsid = req->hasParam("ssid", true);
    bool hasPass = req->hasParam("pass", true);
    bool hasPassProvided = req->hasParam("passProvided", true);
    bool hasUseStatic = req->hasParam("useStatic", true);
    bool hasIp = req->hasParam("ip", true);
    bool hasGw = req->hasParam("gw", true);
    bool hasDns = req->hasParam("dns", true);
    bool hasMask = req->hasParam("mask", true);
    bool hasLedEnable = req->hasParam("ledEnable", true);
    bool hasLedBrightness = req->hasParam("ledBrightness", true);
    bool hasReboot = req->hasParam("reboot", true);

    String ssid = getPostParam(req, "ssid", wifiSsid);
    ssid.trim();
    if (!ssid.length()) {
      req->send(400, "application/json; charset=utf-8",
                buildApiErrorJson("VALIDATION_ERROR", "SSID is required"));
      return;
    }

    String pass = getPostParam(req, "pass", "");
    bool passProvided = parseBoolParam(getPostParam(req, "passProvided", "0"));
    bool useStatic = parseBoolParam(getPostParam(req, "useStatic", wifiUseStatic ? "1" : "0"));
    bool reboot = parseBoolParam(getPostParam(req, "reboot", "0"));
    bool ledEnable = parseBoolParam(getPostParam(req, "ledEnable", gStatusLedEnabled ? "1" : "0"));
    int ledBrightness = getPostParam(req, "ledBrightness", String((int)gStatusLedBrightness)).toInt();
    if (ledBrightness < STATUS_LED_BRIGHTNESS_MIN) ledBrightness = STATUS_LED_BRIGHTNESS_MIN;
    if (ledBrightness > STATUS_LED_BRIGHTNESS_MAX) ledBrightness = STATUS_LED_BRIGHTNESS_MAX;

    String ip = getPostParam(req, "ip", wifiIpStr);
    String gw = getPostParam(req, "gw", wifiGwStr);
    String dns = getPostParam(req, "dns", wifiDnsStr);
    String mask = getPostParam(req, "mask", wifiMaskStr);
    ip.trim();
    gw.trim();
    dns.trim();
    mask.trim();

    if (useStatic) {
      String err;
      if (!validateStaticConfig(ip, gw, dns, mask, err)) {
        req->send(400, "application/json; charset=utf-8",
                  buildApiErrorJson("VALIDATION_ERROR", err));
        return;
      }
    } else {
      ip = "";
      gw = "";
      dns = "";
      mask = "";
    }

    wifiSsid = ssid;
    if (passProvided) wifiPass = pass;
    wifiUseStatic = useStatic;
    wifiIpStr = ip;
    wifiGwStr = gw;
    wifiDnsStr = dns;
    wifiMaskStr = mask;
    gStatusLedEnabled = ledEnable;
    gStatusLedBrightness = (uint8_t)ledBrightness;

    saveWifiCreds(wifiSsid, wifiPass);
    saveWifiStatic(wifiUseStatic, wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr);
    saveStatusLedConfig(gStatusLedEnabled, gStatusLedBrightness);

    String savedFieldsJson = "[";
    bool first = true;
    auto addSavedField = [&](const char* k) {
      if (!first) savedFieldsJson += ",";
      savedFieldsJson += "\"";
      savedFieldsJson += k;
      savedFieldsJson += "\"";
      first = false;
    };
    if (hasSsid) addSavedField("ssid");
    if (hasPass && hasPassProvided && passProvided) addSavedField("pass");
    if (hasUseStatic) addSavedField("useStatic");
    if (hasIp) addSavedField("ip");
    if (hasGw) addSavedField("gw");
    if (hasDns) addSavedField("dns");
    if (hasMask) addSavedField("mask");
    if (hasLedEnable) addSavedField("ledEnable");
    if (hasLedBrightness) addSavedField("ledBrightness");
    if (hasReboot) addSavedField("reboot");
    savedFieldsJson += "]";

    String msg = reboot
      ? "network settings saved; rebooting"
      : "network settings saved (reboot to apply)";
    req->send(200, "application/json; charset=utf-8", buildNetworkConfigJson(msg, "NETWORK_SAVED", savedFieldsJson));
    wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[NET] ") + jsonEscape(msg) + "\"}");
    wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[NET] saved fields: ") + jsonEscape(savedFieldsJson) + "\"}");

    if (reboot) {
      delay(250);
      ESP.restart();
    }
  });
  server.on("/api/update", HTTP_POST,
    [](AsyncWebServerRequest* req) {
      bool ok = gFwUpdateOk && !Update.hasError();
      int code = ok ? 200 : 500;
      String message = ok ? "update successful; rebooting" : (gFwUpdateError.length() ? gFwUpdateError : String("update failed"));
      String body = "{";
      body += "\"ok\":" + String(ok ? "true" : "false") + ",";
      body += "\"code\":\"" + String(ok ? "UPDATE_OK" : "UPDATE_FAILED") + "\",";
      body += "\"message\":\"" + jsonEscape(message) + "\",";
      body += "\"msg\":\"" + jsonEscape(message) + "\"";
      body += "}";
      req->send(code, "application/json; charset=utf-8", body);

      gFwUpdateInProgress = false;
      if (ok) {
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[FW] update successful -> reboot\"}");
        delay(200);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest* req, String filename, size_t index, uint8_t* data, size_t len, bool final) {
      (void)req;
      if (index == 0) {
        gFwUpdateInProgress = true;
        gFwUpdateOk = false;
        gFwUpdateError = "";
        if (gSeqState == SEQ_RUNNING) {
          gFwUpdateError = "cannot update while sequence is running";
          return;
        }
        if (!filename.length()) filename = "firmware.bin";
        wsBroadcastJson(String("{\"type\":\"log\",\"msg\":\"[FW] update upload start: ")
                        + jsonEscape(filename) + "\"}");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          gFwUpdateError = "Update.begin failed";
          Update.printError(Serial);
          return;
        }
      }

      if (gFwUpdateError.length()) return;
      if (len && (Update.write(data, len) != len)) {
        gFwUpdateError = "Update.write failed";
        Update.printError(Serial);
        return;
      }

      if (final) {
        if (!Update.end(true)) {
          gFwUpdateError = "Update.end failed";
          Update.printError(Serial);
          return;
        }
        gFwUpdateOk = true;
      }
    }
  );

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.printf("[HTTP] server started on port %u\n", HTTP_PORT);
  Serial.printf("[HTTP] open: http://%s.local/\n", MDNS_NAME);

  // Initialize sequencer defaults
  seqResetInternal();

  printHelpSerial();
}

void loop() {
  // housekeeping for ws
  ws.cleanupClients();

  // serial
  String cmd = readLineSerial();
  if (cmd.length()) handleSerialCmd(cmd);

  // BLE autoconnect
  static bool lastBle = false;
  if (gReqPhonePairStart) {
    gReqPhonePairStart = false;
    phonePairingStart();
    wsSendStatus();
  }
  if (gReqPhonePairStop) {
    gReqPhonePairStop = false;
    phonePairingStop();
    wsSendStatus();
  }
  if (gTriggerMode == TRIGGER_MODE_HW && gPhoneServer) {
    bool advOn = gPhoneServer->getAdvertising() && gPhoneServer->getAdvertising()->isAdvertising();
    if (gPhonePairing || gPhoneConnected || advOn) {
      phoneDisableForHardwareMode();
      wsSendStatus();
    }
  }
  bleAutoConnectTick();
  bleHeartbeatTick();

  // If we are running but not receiving BLE updates, force a disconnect to trigger recovery
  if (gSeqState == SEQ_RUNNING && gBleConnected) {
    uint32_t nowDisc = millis();
    if ((nowDisc - gLastRxMs) > 2500 && (int32_t)(nowDisc - gForceDiscNextMs) >= 0) {
      wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] no RX -> force BLE reconnect\"}");
      bleDisconnect();
      gBleDisconnectSeen = true;
      gForceDiscNextMs = nowDisc + 2000;
    }
  }

  if (gBleDisconnectSeen) {
    gBleDisconnectSeen = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[BLE] disconnected\"}");
    seqPauseForBleLoss(true);
  }

  if (gPhoneConnectSeen) {
    gPhoneConnectSeen = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[PHONE] smartphone connected\"}");
    seqResumeRecoverAfterPhoneReconnect();
    wsSendStatus();
  }
  if (gPhoneDisconnectSeen) {
    gPhoneDisconnectSeen = false;
    wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[PHONE] smartphone disconnected\"}");
    if (gSeqTriggerMode == TRIGGER_MODE_SMARTPHONE) {
      seqPauseForPhoneLoss(true);
    }
    wsSendStatus();
  }
  // Hard guard: never advertise unless pairing was explicitly requested.
  if (gPhoneServer && gPhoneServer->getAdvertising() && !gPhonePairing && !gPhoneConnected) {
    if (gPhoneServer->getAdvertising()->isAdvertising()) {
      gPhoneServer->getAdvertising()->stop();
    }
  }

  if (lastBle != gBleConnected) {
    lastBle = gBleConnected;
    wsSendStatus();
    if (!gBleConnected) {
      seqPauseForBleLoss(false);
    } else {
      // Always stop motion on reconnect (safe idle)
      bleWriteRaw("+CT,STOP;");
      bleWriteRaw("+CR,STOP;");
      // Start idle monitoring window if no sequence is running
      if (gSeqState == SEQ_IDLE) {
        gIdleMonitorUntilMs = millis() + 10000;
        gIdleNextPollMs = 0;
        gIdleHaveAngle = false;
      }
      seqResumeRecoverAfterReconnect();
    }
  }

  // Safety: if we missed the edge but need recover and BLE is up, start recovery
  seqResumeRecoverAfterReconnect();
  seqResumeRecoverAfterPhoneReconnect();

  // sequencer
  seqTick();
  manualTurnTick();
  manualTiltTick();

  // idle monitor after reconnect: stop auto-rotate if it starts
  if (gSeqState == SEQ_IDLE && gBleConnected && gIdleMonitorUntilMs) {
    uint32_t nowIdle = millis();
    if ((int32_t)(nowIdle - gIdleMonitorUntilMs) >= 0) {
      gIdleMonitorUntilMs = 0;
    } else if ((int32_t)(nowIdle - gIdleNextPollMs) >= 0) {
      bleWriteRaw("+QT,CHANGEANGLE;");
      gIdleNextPollMs = nowIdle + 300;
    }

    if (gAngleUpdated) {
      gAngleUpdated = false;
      if (!gIdleHaveAngle) {
        gIdleHaveAngle = true;
        gIdleLastAngle = gLastAngle;
      } else {
        float dist = angDistDeg(gLastAngle, gIdleLastAngle);
        gIdleLastAngle = gLastAngle;
        if (dist > gRotTolDeg) {
          bleWriteRaw("+CT,STOP;");
          bleWriteRaw("+CR,STOP;");
        }
      }
    }
  }

  // periodically push status (lightweight)
  static uint32_t nextStatus = 0;
  uint32_t now = millis();
  ledTick(now);
  if ((int32_t)(now - nextStatus) >= 0) {
    wsSendStatus();
    nextStatus = now + 500; // 2 Hz
  }

  delay(2);
}
