\
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include <NimBLEDevice.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "web_assets.h"

/*
RealityScanRig3000 – ESP32-S3: UI (WebSocket) + BLE Turntable + Sequencer (v1)

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
static const char* MDNS_NAME = "scanrig"; // -> scanrig.local
static const uint16_t HTTP_PORT = 80;

Preferences prefs;
static String wifiSsid;
static String wifiPass;

// AP fallback
static const char* AP_SSID = "RealityScanRig3000";
static const char* AP_PASS = "scanrig3000";

// ===== Firm profile (project memory) =====
// NOTE: You can overwrite these via Serial commands (stored in NVS).
static const char* FIRM_SSID = "KSTWlan";
static const char* FIRM_PASS = "kst52372kst52372";

// Static IP profile (temporary in company network)
static const char* FIRM_IP   = "10.0.0.232";
static const char* FIRM_GW   = "10.10.10.10";
static const char* FIRM_DNS  = "10.10.10.10";
static const char* FIRM_MASK = "255.255.255.0";

static bool   wifiUseStatic = false;
static String wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr;

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

static void wifiStart() {
  loadWifiCreds();

  // If nothing saved yet, default to firm profile (can be changed via Serial).
  if (!wifiSsid.length()) {
    wifiSsid = FIRM_SSID;
    wifiPass = FIRM_PASS;
    wifiUseStatic = true;
    wifiIpStr = FIRM_IP;
    wifiGwStr = FIRM_GW;
    wifiDnsStr = FIRM_DNS;
    wifiMaskStr = FIRM_MASK;
    saveWifiCreds(wifiSsid, wifiPass);
    saveWifiStatic(wifiUseStatic, wifiIpStr, wifiGwStr, wifiDnsStr, wifiMaskStr);
    Serial.println("[WIFI] no saved creds -> defaulting to FIRM profile (KSTWlan + static IP)");
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
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] connected IP=%s\n", WiFi.localIP().toString().c_str());
      return;
    }
    Serial.println("[WIFI] STA connect failed -> AP fallback");
  } else {
    Serial.println("[WIFI] no ssid -> AP fallback");
  }

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WIFI] AP %s ssid='%s' pass='%s'\n", ok ? "ON" : "FAIL", AP_SSID, AP_PASS);
  Serial.printf("[WIFI] AP IP=%s\n", WiFi.softAPIP().toString().c_str());
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
static void wsLog(const String& msg) {
  String j = String("{\"type\":\"log\",\"msg\":") + "\"" + msg + "\"}";
  // naive JSON escaping for quotes/backslashes/newlines
  j.replace("\\", "\\\\"); // first escape backslashes
  j.replace("\"", "\\\"");
  j.replace("\n", "\\n");
  j.replace("\r", "");
  j = String("{\"type\":\"log\",\"msg\":\"") + j.substring(String("{\"type\":\"log\",\"msg\":").length()+1); // already escaped
  wsBroadcastJson(j);
}

// ===== Turntable BLE =====
static const char* TT_NAME = "REVO_DUAL_AXIS_TABLE";
static const NimBLEUUID TT_SVC_UUID((uint16_t)0xFFE0);
static const NimBLEUUID TT_CHR_UUID((uint16_t)0xFFE1);

static NimBLEClient* gClient = nullptr;
static NimBLERemoteCharacteristic* gChr = nullptr;
static bool gBleConnected = false;

class TTClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient*) override {
    gBleConnected = false;
    gChr = nullptr;
    Serial.println("[BLE] disconnected");
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


static void onNotifyCB(NimBLERemoteCharacteristic*, uint8_t* pData, size_t len, bool) {
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
          gLastAngle = gAsm.substring(p, e).toFloat();
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
  Serial.println("[BLE] connected");
  return true;
}

// ===== Sequencer (same logic as v1) =====
enum SeqState : uint8_t { SEQ_IDLE=0, SEQ_RUNNING, SEQ_PAUSED };
enum SeqSub   : uint8_t { SUB_NONE=0, SUB_TILT_SEND, SUB_TILT_WAIT, SUB_ROT_SEND, SUB_ROT_WAIT, SUB_SNAP, SUB_COOLDOWN, SUB_DONE };

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
static uint32_t gSnapCooldownMs = 500;
static uint32_t gTiltMoveMs = 5000;
static uint32_t gTiltReserveMs = 0;

static int gTiltIdx = 0;
static int gRotIdx  = 0;
static float gStepDeg = 5.0f;

static float gCurTiltTarget = 0.0f;
static float gRotTargetDeg  = 0.0f;
static uint32_t gStateTs = 0;
static uint32_t gNextPollTs = 0;

static uint32_t gTotalSteps = 72 * 9;
static uint32_t gDoneSteps = 0;

static float angDistDeg(float a, float b) {
  float d = fabsf(a - b);
  if (d > 180.0f) d = 360.0f - d;
  return d;
}
static float normAngleDeg(float a) {
  while (a > 180.0f) a -= 360.0f;
  while (a < -180.0f) a += 360.0f;
  return a;
}
static float lerp(float a, float b, float t) { return a + (b - a) * t; }
static float tiltAtIndex(int idx, int steps, float from, float to) {
  if (steps <= 1) return from;
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

  gStateTs = millis();
  gNextPollTs = 0;
}

static void seqStart() {
  if (!gBleConnected) return;
  seqResetInternal();
  gSeqState = SEQ_RUNNING;
  gSub = SUB_TILT_SEND;
  wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] START\"}");
}
static void seqPause() { if (gSeqState == SEQ_RUNNING) { gSeqState = SEQ_PAUSED; wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] PAUSE\"}"); } }
static void seqResume(){ if (gSeqState == SEQ_PAUSED)  { gSeqState = SEQ_RUNNING; wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] RESUME\"}"); } }
static void seqAbort() { if (gSeqState != SEQ_IDLE)    { gSeqState = SEQ_IDLE; gSub = SUB_NONE; wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] ABORT\"}"); } }

static void seqTick() {
  if (gSeqState != SEQ_RUNNING) return;
  if (!gBleConnected) { seqAbort(); return; }

  uint32_t now = millis();

  switch (gSub) {
    case SUB_TILT_SEND: {
      gCurTiltTarget = tiltAtIndex(gTiltIdx, gTiltSteps, gTiltFrom, gTiltTo);
      String cmd = String("+CR,TILTVALUE=") + String(gCurTiltTarget, 1) + ";";
      bleWriteRaw(cmd);
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
      gRotTargetDeg = normAngleDeg(start + gStepDeg);

      gAngleUpdated = false;
      gNextPollTs = now;
      gStateTs = now;

      String cmd = String("+CT,TURNANGLE=") + String(gStepDeg, 1) + ";";
      bleWriteRaw(cmd);

      gSub = SUB_ROT_WAIT;
    } break;

    case SUB_ROT_WAIT: {
      if ((int32_t)(now - gNextPollTs) >= 0) {
        bleWriteRaw("+QT,CHANGEANGLE;");
        gNextPollTs = now + gPollMs;
      }

      if (gAngleUpdated) {
        gAngleUpdated = false;
        float dist = angDistDeg(gLastAngle, gRotTargetDeg);
        if (dist <= gRotTolDeg) gSub = SUB_SNAP;
      }

      if (now - gStateTs > gRotTimeoutMs) {
        wsBroadcastJson("{\"type\":\"log\",\"msg\":\"[SEQ] ROT TIMEOUT -> ABORT\"}");
        seqAbort();
      }
    } break;

    case SUB_SNAP: {
      // Placeholder camera trigger
      // We also broadcast a log line for the UI.
      wsBroadcastJson("{\"type\":\"rigLine\",\"line\":\"SNAP\"}");

      gStateTs = now;
      gSub = SUB_COOLDOWN;
    } break;

    case SUB_COOLDOWN: {
      if (now - gStateTs >= gSnapCooldownMs) {
        gDoneSteps++;

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

static String buildRigStateJson() {
  String state = (gSeqState == SEQ_IDLE) ? "IDLE" : (gSeqState == SEQ_RUNNING ? "RUNNING" : "PAUSED");
  bool bleNow = (gClient && gClient->isConnected());
  gBleConnected = bleNow;


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
  j += "\"ANGLE_LAST\":\"" + String(gLastAngle, 2) + "\",";
  j += "\"BLE\":\"" + String(bleNow ? 1 : 0) + "\",";
  j += "\"TT\":\"" + String(bleNow ? 1 : 0) + "\",";
  j += "\"IP\":\"" + ip + "\",";
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
  } else if (key == "SNAP_COOLDOWN_MS") {
    gSnapCooldownMs = (uint32_t)val.toInt();
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
  wifiUseStatic = true;
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
        static bool     gBleAutoConnect = true;
        static uint32_t gBleNextAttemptMs = 0;
        static uint32_t gBleBackoffMs = 2000;

        static void bleNoteAttempt(bool ok) {
          if (ok) { gBleBackoffMs = 2000; gBleNextAttemptMs = 0; }
          else {
            if (gBleBackoffMs < 30000) gBleBackoffMs = min<uint32_t>(30000, gBleBackoffMs * 2);
            gBleNextAttemptMs = millis() + gBleBackoffMs;
          }
        }

        static void bleAutoConnectTick() {
          if (!gBleAutoConnect) return;

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

  loadWifiCreds();
  wifiStart();
  mdnsStart();

  // BLE init (lazy connect)
  bleInitOnce();

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
  bleAutoConnectTick();
  bleHeartbeatTick();
  if (lastBle != gBleConnected) { lastBle = gBleConnected; wsSendStatus(); }

  // sequencer
  seqTick();

  // periodically push status (lightweight)
  static uint32_t nextStatus = 0;
  uint32_t now = millis();
  if ((int32_t)(now - nextStatus) >= 0) {
    wsSendStatus();
    nextStatus = now + 500; // 2 Hz
  }

  delay(2);
}
