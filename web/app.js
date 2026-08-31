// UI boot marker (helps detect if the browser is running JS at all)
console.log("[scanrig-ui] app.js loaded");

const el = (id) => document.getElementById(id);

const connBadge = el("connBadge");
const tcpBadge  = el("tcpBadge");
const ipBadge   = el("ipBadge");
const ttBadge  = el("ttBadge");
const phoneBtBadge = el("phoneBtBadge");
const tabControl = el("tabControl");
const tabSettings = el("tabSettings");
const settingsSections = Array.from(document.querySelectorAll(".settingsSection"));
const controlSections = Array.from(document.querySelectorAll("main > section.card:not(.settingsSection)"));

const statusGrid = el("statusGrid");
const statePill = el("statePill");
const shotsPill = el("shotsPill");
const seqTimePill = el("seqTimePill");
const btnTriggerToggle = el("btnTriggerToggle");
const actionText = el("actionText");
const actionRow = el("actionRow");
const startWarn = el("startWarn");
const manualControls = el("manualControls");
const manualLockOverlay = el("manualLockOverlay");
const manualLastAction = el("manualLastAction");
const btnTtUp = el("btnTtUp");
const btnTtTiltZero = el("btnTtTiltZero");
const btnTtDown = el("btnTtDown");
const btnTtLeft = el("btnTtLeft");
const btnTtRotZero = el("btnTtRotZero");
const btnTtStop = el("btnTtStop");
const btnTtRight = el("btnTtRight");
const btnSnap = el("btnSnap");
const triggerModeSelect = el("triggerModeSelect");
const btnAutofocusToggle = el("btnAutofocusToggle");
const btnFlashGuardToggle = el("btnFlashGuardToggle");
const inpFlashGuardEvery = el("inpFlashGuardEvery");
const btnSetFlashGuardEvery = el("btnSetFlashGuardEvery");
const inpFlashGuardMs = el("inpFlashGuardMs");
const btnSetFlashGuardMs = el("btnSetFlashGuardMs");
const btnPhonePairToggle = el("btnPhonePairToggle");
const btnPhoneDisconnect = el("btnPhoneDisconnect");
const phoneConnectInfo = el("phoneConnectInfo");
const rotViz = el("rotViz");
const rotNeedle = el("rotNeedle");
const rotMeta = el("rotMeta");
const tiltViz = el("tiltViz");
const tiltNeedle = el("tiltNeedle");
const tiltMeta = el("tiltMeta");

const progressText = el("progressText");
const seqSegments = el("seqSegments");
const fwFile = el("fwFile");
const btnFwUpload = el("btnFwUpload");
const fwUploadState = el("fwUploadState");
const manifestUrlInput = el("manifestUrl");
const btnCheckUpdate = el("btnCheckUpdate");
const btnFwUpdateNow = el("btnFwUpdateNow");
const fwVersionInfo = el("fwVersionInfo");
const appFooterText = el("appFooterText");
const DEFAULT_MANIFEST_URL = "https://superwutz.github.io/RealityScanRig3000/manifest.json";
const networkInfo = el("networkInfo");
const setWifiSsid = el("setWifiSsid");
const setWifiPass = el("setWifiPass");
const setUseStatic = el("setUseStatic");
const staticFields = el("staticFields");
const setIp = el("setIp");
const setGw = el("setGw");
const setDns = el("setDns");
const setMask = el("setMask");
const setLedEnable = el("setLedEnable");
const setLedBrightness = el("setLedBrightness");
const setLedBrightnessVal = el("setLedBrightnessVal");
const btnNetReload = el("btnNetReload");
const btnNetSave = el("btnNetSave");
const btnNetSaveReboot = el("btnNetSaveReboot");
const apModeBanner = el("apModeBanner");
const settingsDirtyHint = el("settingsDirtyHint");
const btnTestFocus = el("btnTestFocus");
const btnTestTrigger = el("btnTestTrigger");
const victoryOverlay = el("victoryOverlay");
const victoryText = el("victoryText");
const btnVictoryClose = el("btnVictoryClose");
const btnVictoryReset = el("btnVictoryReset");

const logEl = el("log");

// Presets UI
const presetExplain = el("presetExplain");
const btnApplyPreset = el("btnApplyPreset");
const btnAutoApplyToggle = el("btnAutoApplyToggle");

const presetButtons = {
  quick: el("presetSmall"),
  medium: el("presetMedium"),
  detailed: el("presetLarge"),
};
const presetActive = {
  quick: el("presetSmallActive"),
  medium: el("presetMediumActive"),
  detailed: el("presetLargeActive"),
};
const presetVals = {
  quick: {
    rotSteps: el("presetSmallRotSteps"),
    tiltSteps: el("presetSmallTiltSteps"),
    deg: el("presetSmallDeg"),
    shots: el("presetSmallShots"),
    time: el("presetSmallTime"),
  },
  medium: {
    rotSteps: el("presetMediumRotSteps"),
    tiltSteps: el("presetMediumTiltSteps"),
    deg: el("presetMediumDeg"),
    shots: el("presetMediumShots"),
    time: el("presetMediumTime"),
  },
  detailed: {
    rotSteps: el("presetLargeRotSteps"),
    tiltSteps: el("presetLargeTiltSteps"),
    deg: el("presetLargeDeg"),
    shots: el("presetLargeShots"),
    time: el("presetLargeTime"),
  },
};

// Quick set (no STEP_TOTAL input)
const inpRotSteps = el("inpRotSteps");
const inpTiltSteps = el("inpTiltSteps");
const btnSetRotSteps = el("btnSetRotSteps");
const btnSetTiltSteps = el("btnSetTiltSteps");
const inpTiltFrom = el("inpTiltFrom");
const inpTiltTo = el("inpTiltTo");
const btnSetTiltFrom = el("btnSetTiltFrom");
const btnSetTiltTo = el("btnSetTiltTo");
const inpSnapSettle = el("inpSnapSettle");
const inpSnapCooldown = el("inpSnapCooldown");
const btnSetSnapSettle = el("btnSetSnapSettle");
const btnSetSnapCooldown = el("btnSetSnapCooldown");

let ws;
let rigState = {};
let tcpConnected = false;

let selectedPresetKey = null; // what user selected next
let triggerEnabled = true;
let autoApplyEnabled = true; // default ON
let currentView = "control";
let networkLoadedOnce = false;
let startWarnTimer = null;
let victoryVisible = false;
    // UI-only for now (dry-run when false)

const LOCKED_KEYS_WHEN_RUNNING = new Set([
  "ROT_STEPS",
  "TILT_STEPS",
  "TILT_FROM",
  "TILT_TO",
]);

let gVizTiltIdx = null;
let gVizRotStartAngle = null;
let gVizRotStartAngleApplied = null;
let gVizRotStepsBuilt = 0;
let gVizRotDisplayStepsBuilt = 0;
let gVizTiltStepsBuilt = 0;
let gVizTiltFromBuilt = null;
let gVizTiltToBuilt = null;
let gVizWasRunning = false;
let gSeqRotStepsBuilt = 0;
let gSeqTiltStepsBuilt = 0;
let gSeqNodes = [];
let gSeqRanges = [];
let gSegIssues = new Map();
let gPrevState = null;
let gPrevTcpConnected = false;
let gPrevBleConnected = null;
let latestManifestVersion = "";
let latestManifestCheckUrl = "";
let latestFirmwareUrl = "";
let settingsBaseline = null;
let settingsDirty = false;
let runStartedAtMs = 0;

const HIDDEN_STATUS_KEYS = new Set([
  "BLE", "TT", "IP", "PORT", "STATE", "SUB",
  "TILT_MOVE_MS", "TILT_RESERVE_MS",
  "SNAP_SETTLE_MS", "SNAP_COOLDOWN_MS",
  "SEQ_EST_TIME",
  "ROT_STEP_MS_AVG",
  "FW_VER", "UI_VER", "UPDATE_URL", "BUILD_GIT", "BUILD_TIME",
  "TRIGGER_MODE", "TRIGGER_ENABLED", "PHONE_BT", "PHONE_PAIRING", "PHONE_NAME", "PHONE_PEER",
  "WIFI_MODE", "HOST",
  "FLASH_GUARD", "FLASH_GUARD_REMAIN_MS", "MANUAL_PREFOCUS_MS", "AF_MODE",
]);
const LABELS = {
  AF_MODE: "AF Mode",
  AF_PREFOCUS_MS: "AF Pre-Focus (ms)",
  AF_SHUTTER_MS: "AF Shutter (ms)",
  AF_POSTFOCUS_MS: "AF Release Hold (ms)",
  SNAP_PRESS_MS: "Manual Shutter (ms)",
  MANUAL_PREFOCUS_MS: "Manual Pre-Focus (ms)",
  FLASH_GUARD: "Flash Guard",
  FLASH_GUARD_EVERY: "Flash Guard Every",
  FLASH_GUARD_MS: "Flash Guard Cooldown (s)",
  FLASH_GUARD_REMAIN_MS: "Flash Guard Remaining (s)",
  ROT_STEP_DEG: "Rot Step (deg)",
  TILT_STEP_DEG: "Tilt Step (deg)",
  ROT_TARGET_DEG: "Rot Target (deg)",
  TILT_TARGET_DEG: "Tilt Target (deg)",
  TILT_WAIT_MS: "Tilt Wait (ms)",
  SNAP_COOLDOWN_MS: "Snap Cooldown (ms)",
  SNAP_SETTLE_MS: "Snap Settle (ms)",
  SEQ_EST_TIME: "Seq Est Time",
  ROT_STEPS: "Rot Steps",
  TILT_STEPS: "Tilt Steps",
  TILT_FROM: "Tilt From (deg)",
  TILT_TO: "Tilt To (deg)",
  ROT_ANGLE: "Rot Angle (deg)",
  TILT_ANGLE: "Tilt Angle (deg)",
  STEP: "Step",
  STATE: "State",
  SUB: "Substate",
};
const SUB_LABELS = {
  0: "Idle",
  1: "Recover",
  2: "Recover (wait)",
  3: "Seek rotation",
  4: "Tilt send",
  5: "Tilt wait",
  6: "Rotate send",
  7: "Rotate wait",
  8: "Settle",
  9: "Trigger",
  10: "Cooldown",
  11: "Done",
  12: "Flash cooldown",
};

function labelForKey(key){
  if (LABELS[key]) return LABELS[key];
  const raw = String(key || "");
  if (!raw) return raw;
  return raw
    .split("_")
    .map(part => {
      if (!part) return part;
      if (part.length <= 2) return part.toUpperCase();
      return part.charAt(0).toUpperCase() + part.slice(1).toLowerCase();
    })
    .join(" ");
}

const PRESETS = {
  quick:    { label: "Quick",    rotSteps: 18,  tiltSteps: 3  },  // 54 shots
  medium:   { label: "Medium",   rotSteps: 72,  tiltSteps: 5  },  // 360 shots
  detailed: { label: "Detailed", rotSteps: 100, tiltSteps: 7  },  // 700 shots
};

const TIMING_STORE_KEY = "scanrig_preset_timing_v1";
let presetTiming = {};
try {
  presetTiming = JSON.parse(localStorage.getItem(TIMING_STORE_KEY) || "{}");
} catch {
  presetTiming = {};
}
const DEFAULT_PRESET_TIMING = {
  quick:    { rotStepMs: 2351.3 },
  medium:   { rotStepMs: 945.4 },
  detailed: { rotStepMs: 728.1 },
};
for (const [k, v] of Object.entries(DEFAULT_PRESET_TIMING)) {
  if (!presetTiming[k] || !(Number(presetTiming[k].rotStepMs) > 0)) {
    presetTiming[k] = { ...v, ts: Date.now() };
  }
}
savePresetTiming();
function savePresetTiming(){
  try { localStorage.setItem(TIMING_STORE_KEY, JSON.stringify(presetTiming)); } catch {}
}

function addLog(line) {
  const ts = new Date().toLocaleTimeString();
  logEl.textContent += `[${ts}] ${line}\n`;
  logEl.scrollTop = logEl.scrollHeight;
}

function setBadge(badge, text, ok) {
  badge.textContent = text;
  badge.style.borderColor = ok ? "var(--ok)" : "var(--bad)";
}

function formatFlashGuardSeconds(v){
  const n = Number(v);
  if (!Number.isFinite(n)) return v;
  return `${Math.max(0, Math.round(n / 1000))}`;
}

function setView(view){
  currentView = view;
  const showControl = view === "control";
  controlSections.forEach((sec) => sec.classList.toggle("hidden", !showControl));
  settingsSections.forEach((sec) => sec.classList.toggle("hidden", showControl));
  if (tabControl) tabControl.classList.toggle("active", showControl);
  if (tabSettings) tabSettings.classList.toggle("active", !showControl);
  if (!showControl && !networkLoadedOnce) {
    loadNetworkSettings();
  }
}

function setNetworkInfo(msg, ok = true){
  if (!networkInfo) return;
  networkInfo.textContent = msg || "";
  networkInfo.style.color = ok ? "var(--muted)" : "var(--bad)";
}

function toggleStaticFields(){
  if (!staticFields || !setUseStatic) return;
  staticFields.classList.toggle("hidden", !setUseStatic.checked);
}

function bool01(v){
  return !!v ? 1 : 0;
}

function readSettingsFormState(){
  return {
    ssid: setWifiSsid ? String(setWifiSsid.value || "").trim() : "",
    useStatic: bool01(setUseStatic && setUseStatic.checked),
    ip: setIp ? String(setIp.value || "").trim() : "",
    gw: setGw ? String(setGw.value || "").trim() : "",
    dns: setDns ? String(setDns.value || "").trim() : "",
    mask: setMask ? String(setMask.value || "").trim() : "",
    ledEnable: bool01(setLedEnable && setLedEnable.checked),
    ledBrightness: setLedBrightness ? Math.max(1, Math.min(255, Number(setLedBrightness.value) || 1)) : 28,
  };
}

function readSettingsBaselineFromCfg(cfg){
  return {
    ssid: String(cfg.ssid || "").trim(),
    useStatic: bool01(!!cfg.useStatic),
    ip: String(cfg.ip || "").trim(),
    gw: String(cfg.gw || "").trim(),
    dns: String(cfg.dns || "").trim(),
    mask: String(cfg.mask || "").trim(),
    ledEnable: bool01(!!cfg.ledEnable),
    ledBrightness: Math.max(1, Math.min(255, Number(cfg.ledBrightness) || 28)),
  };
}

function updateSettingsDirtyState(force = false){
  if (!settingsBaseline) {
    settingsDirty = false;
  } else {
    const current = readSettingsFormState();
    settingsDirty = Object.keys(settingsBaseline).some((k) => String(current[k]) !== String(settingsBaseline[k]));
  }

  if (btnNetSave) btnNetSave.disabled = !settingsDirty;
  if (btnNetSaveReboot) btnNetSaveReboot.disabled = !settingsDirty;

  if (!settingsDirtyHint) return;
  settingsDirtyHint.classList.remove("clean", "dirty");
  settingsDirtyHint.classList.add(settingsDirty ? "dirty" : "clean");
  if (force && !settingsBaseline) {
    settingsDirtyHint.textContent = "No settings loaded yet.";
    return;
  }
  settingsDirtyHint.textContent = settingsDirty
    ? "Unsaved changes detected. Save applies all fields above."
    : "No unsaved changes.";
}

function updateApModeBanner(cfg){
  if (!apModeBanner) return;
  const mode = String(cfg && cfg.wifiMode ? cfg.wifiMode : "").toUpperCase();
  const currentIp = (cfg && cfg.currentIp) ? cfg.currentIp : "192.168.4.1";
  const mdnsHost = (cfg && cfg.mdns) ? `${cfg.mdns}.local` : (rigState.HOST || "scanrig.local");
  const apSsid = (cfg && cfg.apSsid) ? cfg.apSsid : "ScanRig-Setup";
  const lastErr = (cfg && cfg.wifiLastError) ? String(cfg.wifiLastError) : "";
  if (mode === "AP") {
    apModeBanner.textContent = `AP mode active. Connect to '${apSsid}', then open http://${currentIp}/ or http://${mdnsHost}/ to finish Wi-Fi setup.${lastErr ? ` Last STA error: ${lastErr}` : ""}`;
    apModeBanner.classList.remove("hidden");
  } else {
    apModeBanner.classList.add("hidden");
  }
}

function syncLedBrightnessLabel(){
  if (!setLedBrightness || !setLedBrightnessVal) return;
  const v = Math.max(1, Math.min(255, Number(setLedBrightness.value) || 1));
  setLedBrightness.value = String(v);
  setLedBrightnessVal.textContent = String(v);
  updateSettingsDirtyState();
}

function networkBody(reboot){
  const passRaw = setWifiPass ? setWifiPass.value : "";
  const params = new URLSearchParams();
  params.set("ssid", setWifiSsid ? setWifiSsid.value.trim() : "");
  params.set("pass", passRaw);
  params.set("passProvided", passRaw.length ? "1" : "0");
  params.set("useStatic", setUseStatic && setUseStatic.checked ? "1" : "0");
  params.set("ip", setIp ? setIp.value.trim() : "");
  params.set("gw", setGw ? setGw.value.trim() : "");
  params.set("dns", setDns ? setDns.value.trim() : "");
  params.set("mask", setMask ? setMask.value.trim() : "");
  params.set("ledEnable", setLedEnable && setLedEnable.checked ? "1" : "0");
  params.set("ledBrightness", setLedBrightness ? String(Math.max(1, Math.min(255, Number(setLedBrightness.value) || 1))) : "28");
  params.set("reboot", reboot ? "1" : "0");
  return params;
}

function fillNetworkForm(cfg){
  if (!cfg) return;
  if (setWifiSsid) setWifiSsid.value = cfg.ssid || "";
  if (setWifiPass) setWifiPass.value = "";
  if (setUseStatic) setUseStatic.checked = !!cfg.useStatic;
  if (setIp) setIp.value = cfg.ip || "";
  if (setGw) setGw.value = cfg.gw || "";
  if (setDns) setDns.value = cfg.dns || "";
  if (setMask) setMask.value = cfg.mask || "";
  if (setLedEnable) setLedEnable.checked = !!cfg.ledEnable;
  if (setLedBrightness) setLedBrightness.value = String(Math.max(1, Math.min(255, Number(cfg.ledBrightness) || 28)));
  syncLedBrightnessLabel();
  toggleStaticFields();
  const mode = cfg.useStatic ? "Static IP" : "DHCP";
  const currentIp = cfg.currentIp || "-";
  const mdns = cfg.mdns ? `http://${cfg.mdns}.local/` : "-";
  const wifiMode = cfg.wifiMode ? String(cfg.wifiMode).toUpperCase() : "UNKNOWN";
  const ledText = `LED: ${setLedEnable && setLedEnable.checked ? "ON" : "OFF"} @ ${setLedBrightness ? setLedBrightness.value : "-"}`;
  setNetworkInfo(`IP Mode: ${mode} | Wi-Fi Mode: ${wifiMode} | Current IP: ${currentIp} | mDNS: ${mdns} | ${ledText}`, true);
  updateApModeBanner(cfg);
  settingsBaseline = readSettingsBaselineFromCfg(cfg);
  updateSettingsDirtyState(true);
}

async function loadNetworkSettings(){
  try {
    setNetworkInfo("Loading network settings ...", true);
    const res = await fetch("/api/network", { cache: "no-store" });
    const data = await res.json();
    if (!res.ok || !data.ok) {
      throw new Error(data.message || data.msg || "Failed to load network settings");
    }
    fillNetworkForm(data);
    networkLoadedOnce = true;
  } catch (err) {
    setNetworkInfo(`Load failed: ${err.message}`, false);
    addLog(`[NET] load failed: ${err.message}`);
  }
}

async function saveNetworkSettings(reboot){
  try {
    setNetworkInfo("Saving ...", true);
    const res = await fetch("/api/network", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
      body: networkBody(reboot),
    });
    const data = await res.json();
    if (!res.ok || !data.ok) {
      throw new Error(data.message || data.msg || "Save failed");
    }
    const msg = data.message || data.msg || "Saved";
    const savedFields = Array.isArray(data.savedFields) ? data.savedFields.join(", ") : "";
    setNetworkInfo(savedFields ? `${msg} | Fields: ${savedFields}` : msg, true);
    addLog(`[NET] ${msg}`);
    if (savedFields) addLog(`[NET] fields saved: ${savedFields}`);
    if (!reboot) {
      fillNetworkForm(data);
      networkLoadedOnce = true;
    }
  } catch (err) {
    setNetworkInfo(`Save failed: ${err.message}`, false);
    addLog(`[NET] save failed: ${err.message}`);
  }
}

function isTurntableConnected(){
  const ble = String(rigState.BLE ?? "0").toLowerCase();
  return (ble === "1" || ble === "true");
}

function showStartWarning(msg){
  if (!startWarn) return;
  startWarn.textContent = msg || "Turntable not connected.";
  startWarn.classList.remove("hidden");
  if (startWarnTimer) clearTimeout(startWarnTimer);
  startWarnTimer = setTimeout(() => {
    startWarn.classList.add("hidden");
  }, 5000);
}

function showVictory(message){
  if (!victoryOverlay || !victoryText) return;
  victoryText.textContent = message || "Sequence finished successfully.";
  victoryOverlay.classList.remove("hidden");
  victoryVisible = true;
}

function hideVictory(){
  if (!victoryOverlay) return;
  victoryOverlay.classList.add("hidden");
  victoryVisible = false;
}

function clamp01(x){ return Math.max(0, Math.min(1, x)); }

function deriveProgressFromStep(stepStr) {
  const m = (stepStr || "").match(/^(\d+)\s*\/\s*(\d+)$/);
  if (!m) return null;
  const cur = Number(m[1]);
  const tot = Number(m[2]);
  if (!Number.isFinite(cur) || !Number.isFinite(tot) || tot <= 0) return null;
  return { cur, tot, frac: clamp01(cur / tot) };
}

function setFwUploadState(msg) {
  if (fwUploadState) fwUploadState.textContent = String(msg || "");
}

function setFwVersionInfo(msg) {
  if (fwVersionInfo) fwVersionInfo.textContent = String(msg || "");
}

function versionParts(v) {
  const m = String(v || "").trim().match(/^v?(\d+)\.(\d+)\.(\d+)/i);
  if (!m) return null;
  return [Number(m[1]), Number(m[2]), Number(m[3])];
}

function compareVersions(a, b) {
  const pa = versionParts(a);
  const pb = versionParts(b);
  if (!pa || !pb) return 0;
  for (let i = 0; i < 3; i++) {
    if (pa[i] > pb[i]) return 1;
    if (pa[i] < pb[i]) return -1;
  }
  return 0;
}

function firmwareUrlFromManifest(manifest, manifestUrl) {
  const builds = Array.isArray(manifest && manifest.builds) ? manifest.builds : [];
  for (const b of builds) {
    const parts = Array.isArray(b && b.parts) ? b.parts : [];
    for (const p of parts) {
      const path = String((p && p.path) || "");
      const offset = Number(p && p.offset);
      if (!path) continue;
      if (offset === 65536 || /firmware\.bin$/i.test(path)) {
        return new URL(path, manifestUrl).href;
      }
    }
  }
  return "";
}

function uploadFirmwareBlob(blob, filename) {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest();
    const form = new FormData();
    form.append("firmware", blob, filename || "firmware.bin");

    btnFwUpload && (btnFwUpload.disabled = true);
    btnFwUpdateNow && (btnFwUpdateNow.disabled = true);
    setFwUploadState("Uploading 0%");

    xhr.upload.onprogress = (ev) => {
      if (!ev.lengthComputable) return;
      const p = Math.max(0, Math.min(100, Math.round((ev.loaded / ev.total) * 100)));
      setFwUploadState(`Uploading ${p}%`);
    };
    xhr.onerror = () => {
      setFwUploadState("Upload failed (network error)");
      addLog("[FW] OTA upload failed (network error)");
      btnFwUpload && (btnFwUpload.disabled = false);
      if (btnFwUpdateNow) btnFwUpdateNow.disabled = !latestFirmwareUrl;
      reject(new Error("network error"));
    };
    xhr.onload = () => {
      let msg = "";
      try {
        const body = JSON.parse(xhr.responseText || "{}");
        msg = String(body.msg || "");
      } catch {}
      if (xhr.status >= 200 && xhr.status < 300) {
        setFwUploadState("Update accepted, rebooting...");
        addLog("[FW] OTA upload complete, device rebooting");
        resolve();
      } else {
        const errMsg = msg || `Update failed (HTTP ${xhr.status})`;
        setFwUploadState(errMsg);
        addLog(`[FW] OTA upload failed (HTTP ${xhr.status})`);
        btnFwUpload && (btnFwUpload.disabled = false);
        if (btnFwUpdateNow) btnFwUpdateNow.disabled = !latestFirmwareUrl;
        reject(new Error(errMsg));
      }
    };

    xhr.open("POST", "/api/update");
    xhr.send(form);
  });
}

function clearSegmentIssues() {
  gSegIssues = new Map();
}

function issueSeverityWeight(level) {
  if (level === "error") return 2;
  if (level === "warn") return 1;
  return 0;
}

function getCurrentStepContext() {
  const prog = deriveProgressFromStep(rigState.STEP);
  const rotSteps = Number(rigState.ROT_STEPS);
  const tiltSteps = Number(rigState.TILT_STEPS);
  if (!prog || !Number.isFinite(rotSteps) || rotSteps <= 0 || !Number.isFinite(tiltSteps) || tiltSteps <= 0) return null;
  const total = Math.max(1, rotSteps * tiltSteps);
  const active = Math.max(0, Math.min(total - 1, Number(prog.cur) || 0));
  return { total, active };
}

function markIssueAtCurrentSegment(level, reason) {
  const ctx = getCurrentStepContext();
  if (!ctx) return;
  const prev = gSegIssues.get(ctx.active);
  if (!prev || issueSeverityWeight(level) > issueSeverityWeight(prev.level)) {
    gSegIssues.set(ctx.active, { level, reason: String(reason || "") });
  }
}

function segmentIssueLevel(rawStart, rawEnd) {
  let level = "";
  for (const [idx, issue] of gSegIssues.entries()) {
    if (idx < rawStart || idx >= rawEnd) continue;
    if (issue && issue.level === "error") return "error";
    if (issue && issue.level === "warn") level = "warn";
  }
  return level;
}

function flashGuardConfig() {
  const mode = String(rigState.TRIGGER_MODE || "HARDWARE").toUpperCase();
  const raw = String(rigState.FLASH_GUARD ?? "0").toLowerCase();
  const enabled = (mode === "HARDWARE") && !(raw === "0" || raw === "false");
  const every = Number(rigState.FLASH_GUARD_EVERY);
  const rotSteps = Number(rigState.ROT_STEPS);
  const tiltSteps = Number(rigState.TILT_STEPS);
  return {
    enabled: enabled && Number.isFinite(every) && every > 0 && Number.isFinite(rotSteps) && rotSteps > 0 && Number.isFinite(tiltSteps) && tiltSteps > 0,
    every: Number.isFinite(every) ? Math.max(1, Math.round(every)) : 0,
    totalShots: (Number.isFinite(rotSteps) && rotSteps > 0 && Number.isFinite(tiltSteps) && tiltSteps > 0) ? Math.round(rotSteps * tiltSteps) : 0,
    rotSteps: Number.isFinite(rotSteps) ? Math.max(1, Math.round(rotSteps)) : 1,
  };
}

function shotTriggersFlashPause(shotNumber1Based, cfg) {
  if (!cfg || !cfg.enabled || cfg.every <= 0 || cfg.totalShots <= 1) return false;
  return shotNumber1Based > 0 &&
         shotNumber1Based < cfg.totalShots &&
         (shotNumber1Based % cfg.every) === 0;
}

function segmentHasFlash(rawStart, rawEnd) {
  const cfg = flashGuardConfig();
  if (!cfg.enabled) return false;
  const a = Math.max(0, Math.floor(rawStart));
  const b = Math.max(a + 1, Math.floor(rawEnd));
  for (let i = a; i < b; i++) {
    if (shotTriggersFlashPause(i + 1, cfg)) return true;
  }
  return false;
}

function detectAndMarkIssueFromText(text) {
  const msg = String(text || "");
  if (!msg) return;
  const ml = msg.toLowerCase();
  if (ml.includes("[ble]") && /(disconnect|disconnected|fail|lost|timeout)/.test(ml)) {
    markIssueAtCurrentSegment("error", "ble event");
    return;
  }
  if (ml.includes("[tcp]") && /(disconnect|disconnected|fail|lost|timeout)/.test(ml)) {
    markIssueAtCurrentSegment("error", "tcp event");
    return;
  }
  if (ml.includes("[wifi]") && /(disconnect|disconnected|fail|lost|timeout)/.test(ml)) {
    markIssueAtCurrentSegment("warn", "wifi event");
  }
}

function computeShots(rotSteps, tiltSteps){
  const r = Number(rotSteps);
  const t = Number(tiltSteps);
  if (!Number.isFinite(r) || !Number.isFinite(t) || r <= 0 || t <= 0) return null;
  return { perTilt: r, total: r * t };
}

function tiltAtIndexJS(idx, steps, from, to){
  const s = Number(steps);
  if (!Number.isFinite(s) || s <= 1) return (Number(from) + Number(to)) * 0.5;
  const t = Number(idx) / (s - 1);
  return Number(from) + (Number(to) - Number(from)) * t;
}

function pointOnCircle(cx, cy, r, deg) {
  const rad = (deg - 90) * (Math.PI / 180);
  return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
}

function donutSlicePath(cx, cy, rOuter, rInner, startDeg, endDeg) {
  let a0 = Number(startDeg);
  let a1 = Number(endDeg);
  while (a1 <= a0) a1 += 360;
  const sweep = a1 - a0;
  const largeArc = sweep > 180 ? 1 : 0;

  const p0 = pointOnCircle(cx, cy, rOuter, a0);
  const p1 = pointOnCircle(cx, cy, rOuter, a1);
  const q1 = pointOnCircle(cx, cy, rInner, a1);
  const q0 = pointOnCircle(cx, cy, rInner, a0);

  return [
    `M ${p0.x.toFixed(3)} ${p0.y.toFixed(3)}`,
    `A ${rOuter} ${rOuter} 0 ${largeArc} 1 ${p1.x.toFixed(3)} ${p1.y.toFixed(3)}`,
    `L ${q1.x.toFixed(3)} ${q1.y.toFixed(3)}`,
    `A ${rInner} ${rInner} 0 ${largeArc} 0 ${q0.x.toFixed(3)} ${q0.y.toFixed(3)}`,
    "Z",
  ].join(" ");
}

function clearVizSegments(svgEl, className) {
  if (!svgEl) return;
  svgEl.querySelectorAll(`.${className}`).forEach((n) => n.remove());
}

function clearVizMarkers(svgEl) {
  if (!svgEl) return;
  svgEl.querySelectorAll(".vizTick,.vizLabel").forEach((n) => n.remove());
}

function addSvgLine(svgEl, x1, y1, x2, y2, cls) {
  const n = document.createElementNS("http://www.w3.org/2000/svg", "line");
  n.setAttribute("x1", String(x1));
  n.setAttribute("y1", String(y1));
  n.setAttribute("x2", String(x2));
  n.setAttribute("y2", String(y2));
  n.setAttribute("class", cls);
  svgEl.appendChild(n);
}

function addSvgText(svgEl, x, y, text, cls, anchor = "middle") {
  const n = document.createElementNS("http://www.w3.org/2000/svg", "text");
  n.setAttribute("x", String(x));
  n.setAttribute("y", String(y));
  n.setAttribute("class", cls);
  n.setAttribute("text-anchor", anchor);
  n.textContent = text;
  svgEl.appendChild(n);
}

function formatDegShort(v) {
  if (!Number.isFinite(v)) return "-";
  const iv = Math.round(v * 10) / 10;
  if (Math.abs(iv - Math.round(iv)) < 1e-6) return String(Math.round(iv));
  return String(iv);
}

function nowClockText() {
  const d = new Date();
  const hh = String(d.getHours()).padStart(2, "0");
  const mm = String(d.getMinutes()).padStart(2, "0");
  const ss = String(d.getSeconds()).padStart(2, "0");
  return `${hh}:${mm}:${ss}`;
}

function setManualLastAction(text) {
  const msg = text && String(text).trim() ? String(text).trim() : "-";
  if (manualLastAction) manualLastAction.textContent = `Last Manual Action: ${msg}`;
  try { localStorage.setItem("scanrig.manual.lastAction", msg); } catch {}
}

function rotDisplaySteps(rawSteps) {
  const rs = Number(rawSteps);
  if (!Number.isFinite(rs) || rs <= 0) return 0;
  if (rs <= 120) return Math.max(1, Math.round(rs));
  return 120;
}

function bringNeedlesToFront() {
  if (rotViz) {
    const rn = rotViz.querySelector("#rotNeedle");
    const rh = rotViz.querySelector(".vizHub");
    if (rn) rotViz.appendChild(rn);
    if (rh) rotViz.appendChild(rh);
  }
  if (tiltViz) {
    const tn = tiltViz.querySelector("#tiltNeedle");
    const th = tiltViz.querySelector(".vizHub");
    if (tn) tiltViz.appendChild(tn);
    if (th) tiltViz.appendChild(th);
  }
}

function buildRotSegments(rotSteps, startAngleDeg) {
  if (!rotViz || !Number.isFinite(rotSteps) || rotSteps <= 0) return;
  clearVizSegments(rotViz, "vizSeg");
  clearVizMarkers(rotViz);

  const displaySteps = rotDisplaySteps(rotSteps);
  if (!Number.isFinite(displaySteps) || displaySteps <= 0) return;
  const base = Number.isFinite(startAngleDeg) ? startAngleDeg : -90;
  const stepDeg = 360 / displaySteps;
  const gapDeg = Math.max(0, Math.min(1.0, stepDeg * 0.18));
  for (let i = 0; i < displaySteps; i++) {
    const a0 = base + (i * stepDeg);
    const a1 = base + ((i + 1) * stepDeg) - gapDeg;
    const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.setAttribute("d", donutSlicePath(60, 60, 48, 40, a0, a1));
    path.setAttribute("class", "vizSeg vizSegTodo");
    path.dataset.idx = String(i);
    rotViz.appendChild(path);
  }

  gVizRotStepsBuilt = rotSteps;
  gVizRotDisplayStepsBuilt = displaySteps;
  gVizRotStartAngleApplied = base;
  bringNeedlesToFront();
}

function updateRotSegmentsDone(rotDoneRaw, activeRawIdx, blinkActive, rawSteps) {
  if (!rotViz) return;
  const raw = Math.max(1, Number(rawSteps) || 1);
  const display = Math.max(1, gVizRotDisplayStepsBuilt || rotDisplaySteps(raw));
  const cfg = flashGuardConfig();
  const tiltIdx = Number.isFinite(gVizTiltIdx) ? Math.max(0, Math.round(gVizTiltIdx)) : 0;
  const globalOffset = tiltIdx * cfg.rotSteps;
  const doneRaw = Math.max(0, Math.min(raw, Number(rotDoneRaw) || 0));
  const activeRaw = Number.isFinite(activeRawIdx) ? Math.max(0, Math.min(raw - 1, Number(activeRawIdx))) : -1;
  const done = Math.max(0, Math.min(display, Math.floor((doneRaw / raw) * display)));
  const active = activeRaw >= 0 ? Math.min(display - 1, Math.floor((activeRaw / raw) * display)) : -1;
  rotViz.querySelectorAll(".vizSeg").forEach((node) => {
    const idx = Number(node.dataset.idx || -1);
    const rawStartLocal = Math.floor((idx * raw) / display);
    const rawEndLocal = Math.max(rawStartLocal + 1, Math.floor(((idx + 1) * raw) / display));
    const rawStart = globalOffset + rawStartLocal;
    const rawEnd = globalOffset + rawEndLocal;
    const issue = segmentIssueLevel(rawStart, rawEnd);
    const hasFlash = segmentHasFlash(rawStart, rawEnd);
    const isDone = idx >= 0 && idx < done;
    const isActive = idx === active && done < display;
    node.classList.toggle("vizSegDone", isDone);
    node.classList.toggle("vizSegActive", isActive && !!blinkActive);
    node.classList.toggle("vizSegWarn", issue === "warn");
    node.classList.toggle("vizSegError", issue === "error");
    node.classList.toggle("vizSegFlash", hasFlash);
    node.classList.toggle("vizSegTodo", !isDone && !(isActive && !!blinkActive) && !issue);
  });
}

function buildTiltSegments(tiltSteps, tiltFrom, tiltTo) {
  if (!tiltViz || !Number.isFinite(tiltSteps) || tiltSteps <= 0) return;
  clearVizSegments(tiltViz, "vizTiltSeg");
  clearVizMarkers(tiltViz);

  if (tiltSteps <= 1) {
    const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.setAttribute("d", donutSlicePath(80, 100, 64, 56, -8, 8));
    path.setAttribute("class", "vizTiltSeg vizSegTodo");
    path.dataset.idx = "0";
    tiltViz.appendChild(path);

    addSvgText(tiltViz, "80", "29", `${formatDegShort((Number(tiltFrom) + Number(tiltTo)) * 0.5)} deg`, "vizLabel");

    gVizTiltStepsBuilt = tiltSteps;
    gVizTiltFromBuilt = Number(tiltFrom);
    gVizTiltToBuilt = Number(tiltTo);
    bringNeedlesToFront();
    return;
  }

  const segCount = Math.max(1, tiltSteps - 1);
  const gapDeg = 1.2;
  for (let i = 0; i < segCount; i++) {
    let a0 = -90 + (i * 180 / segCount);
    let a1 = -90 + ((i + 1) * 180 / segCount);
    if (i > 0) a0 += gapDeg * 0.5;
    if (i < segCount - 1) a1 -= gapDeg * 0.5;

    const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.setAttribute("d", donutSlicePath(80, 100, 64, 56, a0, a1));
    path.setAttribute("class", "vizTiltSeg vizSegTodo");
    path.dataset.idx = String(i);
    tiltViz.appendChild(path);
  }

  for (let i = 0; i < tiltSteps; i++) {
    const a = -90 + (i * 180 / (tiltSteps - 1));
    const pt0 = pointOnCircle(80, 100, 65, a);
    const pt1 = pointOnCircle(80, 100, 69, a);
    addSvgLine(tiltViz, pt0.x.toFixed(2), pt0.y.toFixed(2), pt1.x.toFixed(2), pt1.y.toFixed(2), "vizTick");

    const pl = pointOnCircle(80, 100, 72, a);
    const deg = tiltAtIndexJS(i, tiltSteps, tiltFrom, tiltTo);
    const anchor = (i === 0) ? "start" : (i === tiltSteps - 1 ? "end" : "middle");
    addSvgText(tiltViz, pl.x.toFixed(2), pl.y.toFixed(2), `${formatDegShort(deg)} deg`, "vizLabel", anchor);
  }

  gVizTiltStepsBuilt = tiltSteps;
  gVizTiltFromBuilt = Number(tiltFrom);
  gVizTiltToBuilt = Number(tiltTo);
  bringNeedlesToFront();
}

function updateTiltSegmentsDone(doneSegs, activeSegIdx, blinkActive) {
  if (!tiltViz) return;
  const done = Math.max(0, Number(doneSegs) || 0);
  const active = Number.isFinite(activeSegIdx) ? Math.max(0, Number(activeSegIdx)) : -1;
  tiltViz.querySelectorAll(".vizTiltSeg").forEach((node) => {
    const idx = Number(node.dataset.idx || -1);
    const isDone = idx >= 0 && idx < done;
    const isActive = idx === active;
    node.classList.toggle("vizSegDone", isDone);
    node.classList.toggle("vizSegActive", isActive && !!blinkActive);
    node.classList.toggle("vizSegTodo", !isDone && !(isActive && !!blinkActive));
  });
}

function clearSequenceSegments() {
  if (!seqSegments) return;
  seqSegments.innerHTML = "";
  gSeqNodes = [];
  gSeqRanges = [];
  gSeqRotStepsBuilt = 0;
  gSeqTiltStepsBuilt = 0;
}

function buildSequenceSegments(rotSteps, tiltSteps) {
  if (!seqSegments) return;
  if (!Number.isFinite(rotSteps) || rotSteps <= 0 || !Number.isFinite(tiltSteps) || tiltSteps <= 0) {
    clearSequenceSegments();
    return;
  }

  seqSegments.innerHTML = "";
  gSeqNodes = [];
  gSeqRanges = [];
  const maxTotalCells = 220;
  const maxPerTilt = 18;
  const cellsPerTilt = Math.max(1, Math.min(maxPerTilt, Math.floor(maxTotalCells / tiltSteps)));
  for (let t = 0; t < tiltSteps; t++) {
    const group = document.createElement("div");
    group.className = "seqTiltGroup";

    const row = document.createElement("div");
    row.className = "seqRotRow";
    const bins = Math.max(1, Math.min(rotSteps, cellsPerTilt));
    for (let r = 0; r < bins; r++) {
      const rawStart = (t * rotSteps) + Math.floor((r * rotSteps) / bins);
      const rawEnd = (t * rotSteps) + Math.floor(((r + 1) * rotSteps) / bins);
      const seg = document.createElement("span");
      seg.className = "seqSeg";
      row.appendChild(seg);
      gSeqNodes.push(seg);
      gSeqRanges.push({ rawStart, rawEnd: Math.max(rawStart + 1, rawEnd) });
    }
    group.appendChild(row);
    seqSegments.appendChild(group);
  }

  gSeqRotStepsBuilt = rotSteps;
  gSeqTiltStepsBuilt = tiltSteps;
}

function updateSequenceSegments(done, activeIdx, blinkActive) {
  if (!seqSegments || !gSeqNodes.length || gSeqRanges.length !== gSeqNodes.length) return;
  const doneSafe = Math.max(0, Number(done) || 0);
  const activeSafe = Number.isFinite(activeIdx) ? Math.max(0, Number(activeIdx)) : -1;
  for (let i = 0; i < gSeqNodes.length; i++) {
    const seg = gSeqNodes[i];
    const range = gSeqRanges[i];
    const issue = segmentIssueLevel(range.rawStart, range.rawEnd);
    const hasFlash = segmentHasFlash(range.rawStart, range.rawEnd);
    const isDone = doneSafe >= range.rawEnd;
    const isActive = !isDone && activeSafe >= range.rawStart && activeSafe < range.rawEnd;
    seg.classList.toggle("seqSegDone", isDone);
    seg.classList.toggle("seqSegActive", isActive && !!blinkActive);
    seg.classList.toggle("seqSegWarn", issue === "warn");
    seg.classList.toggle("seqSegError", issue === "error");
    seg.classList.toggle("seqSegFlash", hasFlash);
  }
}

function degPerStep(rotSteps){
  const r = Number(rotSteps);
  if (!Number.isFinite(r) || r <= 0) return null;
  return 360 / r;
}

function formatDeg(x){
  if (!Number.isFinite(x)) return "-";
  const v = Math.round(x * 1000) / 1000;
  return `${v}`;
}

function formatSeconds(ms){
  if (!Number.isFinite(ms)) return "-";
  return `${(ms / 1000).toFixed(1)}s`;
}

function formatDuration(ms){
  if (!Number.isFinite(ms) || ms < 0) return "-";
  const totalSec = Math.round(ms / 1000);
  const h = Math.floor(totalSec / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const s = totalSec % 60;
  if (h > 0) return `${h}h ${m}m ${s}s`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

function detectActivePresetFromRig(){
  const r = Number(rigState?.ROT_STEPS);
  const t = Number(rigState?.TILT_STEPS);
  if (!Number.isFinite(r) || !Number.isFinite(t)) return null;

  for (const [key, p] of Object.entries(PRESETS)){
    if (p.rotSteps === r && p.tiltSteps === t) return key;
  }
  return null;
}

function computeTimeParts(presetKey){
  const snapSettleMs = Number(rigState.SNAP_SETTLE_MS);
  const snapCooldownMs = Number(rigState.SNAP_COOLDOWN_MS);
  const tiltMoveMs = Number(rigState.TILT_MOVE_MS);
  const tiltReserveMs = Number(rigState.TILT_RESERVE_MS);
  const tiltWaitMs = (Number.isFinite(tiltMoveMs) ? tiltMoveMs : 0) + (Number.isFinite(tiltReserveMs) ? tiltReserveMs : 0);
  const afMode = String(rigState.AF_MODE || "AUTO").toUpperCase();
  const afPrefocusMs = Number(rigState.AF_PREFOCUS_MS);
  const afShutterMs = Number(rigState.AF_SHUTTER_MS);
  const afPostfocusMs = Number(rigState.AF_POSTFOCUS_MS);
  const manualPrefocusMs = Number(rigState.MANUAL_PREFOCUS_MS);
  const manualShutterMs = Number(rigState.SNAP_PRESS_MS);
  const flashGuardRaw = String(rigState.FLASH_GUARD ?? "0").toLowerCase();
  const triggerMode = String(rigState.TRIGGER_MODE || "HARDWARE").toUpperCase();
  const flashGuardEnabled = (triggerMode === "HARDWARE") && !(flashGuardRaw === "0" || flashGuardRaw === "false");
  const flashGuardEvery = Number(rigState.FLASH_GUARD_EVERY);
  const flashGuardMs = Number(rigState.FLASH_GUARD_MS);

  const preset = presetTiming[presetKey] || DEFAULT_PRESET_TIMING[presetKey] || {};
  const rotStepMsBaseline = Number(preset.rotStepMs) || 0;
  const rotAvg = Number(rigState.ROT_STEP_MS_AVG);
  const rotStepMs = rotAvg > 0 ? Math.max(rotAvg, rotStepMsBaseline) : rotStepMsBaseline;
  let triggerMs = 0;
  if (afMode === "MANUAL") {
    triggerMs =
      (Number.isFinite(manualPrefocusMs) ? manualPrefocusMs : 0) +
      (Number.isFinite(manualShutterMs) ? manualShutterMs : 0);
  } else {
    triggerMs =
      (Number.isFinite(afPrefocusMs) ? afPrefocusMs : 0) +
      (Number.isFinite(afShutterMs) ? afShutterMs : 0) +
      (Number.isFinite(afPostfocusMs) ? afPostfocusMs : 0);
  }
  const perShotMs = (Number.isFinite(snapSettleMs) ? snapSettleMs : 0) +
                    (Number.isFinite(snapCooldownMs) ? snapCooldownMs : 0) +
                    triggerMs +
                    rotStepMs;
  return {
    tiltWaitMs,
    perShotMs,
    flashGuardEnabled,
    flashGuardEvery: Number.isFinite(flashGuardEvery) ? Math.max(1, Math.round(flashGuardEvery)) : 0,
    flashGuardMs: Number.isFinite(flashGuardMs) ? Math.max(0, Math.round(flashGuardMs)) : 0,
  };
}

function estimatePresetTime(presetKey, rotSteps, tiltSteps){
  const parts = computeTimeParts(presetKey);
  const totalMs = (tiltSteps * parts.tiltWaitMs) + (rotSteps * tiltSteps * parts.perShotMs);
  return totalMs > 0 ? totalMs : null;
}

function presetKeyForSteps(rotSteps, tiltSteps){
  if (!Number.isFinite(rotSteps) || !Number.isFinite(tiltSteps)) return null;
  for (const [key, p] of Object.entries(PRESETS)){
    if (p.rotSteps === rotSteps && p.tiltSteps === tiltSteps) return key;
  }
  return null;
}

function renderPresetSelection(){
  for (const k of Object.keys(presetButtons)){
    const b = presetButtons[k];
    if (b) b.classList.toggle("selected", k === selectedPresetKey);
  }
}

function renderPresetActive(){
  const activeKey = detectActivePresetFromRig();
  for (const k of Object.keys(presetButtons)){
    const b = presetButtons[k];
    if (b) b.classList.toggle("activeRig", k === activeKey);

    const a = presetActive[k];
    if (!a) continue;

    if (k === activeKey){
      a.textContent = "Active";
      a.classList.add("active");
    } else {
      a.textContent = "";
      a.classList.remove("active");
    }
  }
}

function renderPresetEffective(){
  for (const [key, p] of Object.entries(PRESETS)){
    const v = presetVals[key];
    if (!v) continue;
    const deg = degPerStep(p.rotSteps);
    if (v.rotSteps) v.rotSteps.textContent = `${p.rotSteps}`;
    if (v.tiltSteps) v.tiltSteps.textContent = `${p.tiltSteps}`;
    if (v.deg) v.deg.textContent = `${formatDeg(deg)}`;
    const shots = computeShots(p.rotSteps, p.tiltSteps);
    if (v.shots) v.shots.textContent = shots ? `${shots.total}` : "-";
    const estMs = estimatePresetTime(key, p.rotSteps, p.tiltSteps);
    if (v.time) v.time.textContent = estMs ? formatDuration(estMs) : "-";
  }
}

function selectPreset(key){
  selectedPresetKey = key;
  renderPresetSelection();

  const p = PRESETS[key];
  const deg = degPerStep(p.rotSteps);

  if (presetExplain) presetExplain.textContent =
    `Selected: ${p.label} -> ROT_STEPS=${p.rotSteps} (~${formatDeg(deg)} deg/step), TILT_STEPS=${p.tiltSteps}`;

  if (autoApplyEnabled) applySelectedPreset();
}

function sendCmd(line) {
  if (!ws || ws.readyState !== 1) return;
  ws.send(JSON.stringify({ type: "cmd", line }));
}

function canEditParamKey(key){
  const k = String(key || "").trim().toUpperCase();
  if (!k) return false;
  if (rigState.STATE !== "RUNNING") return true;
  return !LOCKED_KEYS_WHEN_RUNNING.has(k);
}

function applySelectedPreset(){
  if (!selectedPresetKey) return;
  if (!canEditParamKey("ROT_STEPS")) return;
  const p = PRESETS[selectedPresetKey];
  sendCmd(`SET ROT_STEPS=${p.rotSteps}`);
  sendCmd(`SET TILT_STEPS=${p.tiltSteps}`);
  sendCmd("STATUS");
}

function updateAutoApplyToggle(){
  if (!btnAutoApplyToggle) return;
  btnAutoApplyToggle.textContent = `Auto-Apply: ${autoApplyEnabled ? "ON" : "OFF"}`;
  btnAutoApplyToggle.classList.toggle("off", !autoApplyEnabled);
  btnAutoApplyToggle.classList.toggle("on", autoApplyEnabled);
}

function updateTriggerToggle(){
  if (!btnTriggerToggle) return;
  btnTriggerToggle.textContent = `Trigger: ${triggerEnabled ? "ON" : "OFF"}`;
  btnTriggerToggle.classList.toggle("off", !triggerEnabled);
  btnTriggerToggle.classList.toggle("on", triggerEnabled);
}

function populateQuickInputs(){
  if (inpRotSteps && (!inpRotSteps.value)) inpRotSteps.placeholder = rigState.ROT_STEPS || "-";
  if (inpTiltSteps && (!inpTiltSteps.value)) inpTiltSteps.placeholder = rigState.TILT_STEPS || "-";
  if (inpTiltFrom && (!inpTiltFrom.value)) inpTiltFrom.placeholder = rigState.TILT_FROM || "-";
  if (inpTiltTo && (!inpTiltTo.value)) inpTiltTo.placeholder = rigState.TILT_TO || "-";
  if (inpSnapSettle && (!inpSnapSettle.value)) inpSnapSettle.placeholder = rigState.SNAP_SETTLE_MS || "-";
  if (inpSnapCooldown && (!inpSnapCooldown.value)) inpSnapCooldown.placeholder = rigState.SNAP_COOLDOWN_MS || "-";
  if (inpFlashGuardEvery && (!inpFlashGuardEvery.value)) inpFlashGuardEvery.placeholder = rigState.FLASH_GUARD_EVERY || "-";
  if (inpFlashGuardMs && (!inpFlashGuardMs.value)) inpFlashGuardMs.placeholder = formatFlashGuardSeconds(rigState.FLASH_GUARD_MS);
}

function updateTurntableBadge() {
  if (!ttBadge) return;
  const ble = String(rigState.BLE ?? "0");
  const on = (ble === "1" || ble.toLowerCase() === "true");
  setBadge(ttBadge, `TT: ${on ? "CONNECTED" : "DISCONNECTED"}`, on);
  ttBadge.classList.toggle("on", on);
  ttBadge.classList.toggle("off", !on);
}

function updateTriggerBackendUi() {
  const mode = String(rigState.TRIGGER_MODE || "HARDWARE").toUpperCase();
  const afMode = String(rigState.AF_MODE || "AUTO").toUpperCase();
  const flashGuardRaw = String(rigState.FLASH_GUARD ?? "1").toLowerCase();
  const phoneConnectedRaw = String(rigState.PHONE_BT ?? "0").toLowerCase();
  const pairingRaw = String(rigState.PHONE_PAIRING ?? "0").toLowerCase();
  const phoneConnected = (phoneConnectedRaw === "1" || phoneConnectedRaw === "true");
  const pairing = (pairingRaw === "1" || pairingRaw === "true");
  const phoneName = String(rigState.PHONE_NAME || "RealityScanRig3000 Remote");
  const phonePeer = String(rigState.PHONE_PEER || "").trim();
  const phoneMode = (mode === "SMARTPHONE");
  const afAuto = (afMode !== "MANUAL");
  const flashGuardEnabled = !(flashGuardRaw === "0" || flashGuardRaw === "false");

  if (triggerModeSelect) {
    triggerModeSelect.value = phoneMode ? "SMARTPHONE" : "HARDWARE";
    triggerModeSelect.title = `Current trigger backend: ${triggerModeSelect.value}`;
  }
  if (phoneBtBadge) {
    if (!phoneMode) {
      phoneBtBadge.textContent = "Phone BT: DISABLED";
      phoneBtBadge.style.borderColor = "var(--line2)";
      phoneBtBadge.classList.add("off");
      phoneBtBadge.title = "Phone Bluetooth is disabled in Hardware trigger mode";
    } else {
      const text = phoneConnected ? "CONNECTED" : (pairing ? "PAIRING" : "IDLE");
      setBadge(phoneBtBadge, `Phone BT: ${text}`, phoneConnected || pairing);
      phoneBtBadge.classList.remove("off");
      const peerInfo = phonePeer ? `\nConnected peer (BLE): ${phonePeer}` : "";
      phoneBtBadge.title = `Bluetooth advertising name: ${phoneName}${peerInfo}`;
    }
    phoneBtBadge.style.display = "";
  }
  if (btnPhonePairToggle) {
    btnPhonePairToggle.textContent = pairing ? "Pair Stop" : "Pair Start";
    btnPhonePairToggle.disabled = !phoneMode;
    btnPhonePairToggle.style.display = phoneMode ? "" : "none";
    btnPhonePairToggle.title = pairing ? "Disable Bluetooth pairing mode" : "Enable Bluetooth pairing mode";
  }
  if (btnPhoneDisconnect) {
    btnPhoneDisconnect.disabled = !phoneMode || !phoneConnected;
    btnPhoneDisconnect.style.display = phoneMode ? "" : "none";
  }
  if (btnAutofocusToggle) {
    btnAutofocusToggle.textContent = `AF: ${afAuto ? "AUTO" : "MANUAL"}`;
    btnAutofocusToggle.style.display = phoneMode ? "none" : "";
    btnAutofocusToggle.title = afAuto
      ? "Hardware trigger autofocus mode is AUTO"
      : "Hardware trigger autofocus mode is MANUAL";
  }
  if (btnFlashGuardToggle) {
    btnFlashGuardToggle.textContent = `Flash Guard: ${flashGuardEnabled ? "ON" : "OFF"}`;
    btnFlashGuardToggle.style.display = phoneMode ? "none" : "";
    const desc = "Insert a long cooldown every N shots (hardware trigger only)";
    btnFlashGuardToggle.title = `${desc} | State: ${flashGuardEnabled ? "enabled" : "disabled"}`;
  }
  const showFlashGuardControls = (!phoneMode) && flashGuardEnabled;
  if (inpFlashGuardEvery) inpFlashGuardEvery.style.display = showFlashGuardControls ? "" : "none";
  if (btnSetFlashGuardEvery) btnSetFlashGuardEvery.style.display = showFlashGuardControls ? "" : "none";
  if (inpFlashGuardMs) inpFlashGuardMs.style.display = showFlashGuardControls ? "" : "none";
  if (btnSetFlashGuardMs) btnSetFlashGuardMs.style.display = showFlashGuardControls ? "" : "none";
  if (phoneConnectInfo) {
    if (!phoneMode) {
      phoneConnectInfo.style.display = "none";
    } else {
      phoneConnectInfo.style.display = "";
      if (phoneConnected) {
        phoneConnectInfo.textContent = phonePeer
          ? `Status: connected peer (BLE): ${phonePeer}`
          : "Status: connected";
      } else if (pairing) {
        phoneConnectInfo.textContent = "Status: pairing active";
      } else {
        phoneConnectInfo.textContent = "Status: idle";
      }
    }
  }
}

function renderStatus() {
  statusGrid.innerHTML = "";
  const st = rigState.STATE || "???";
  const bleNowRaw = String(rigState.BLE ?? "0").toLowerCase();
  const bleNow = (bleNowRaw === "1" || bleNowRaw === "true");
  const stepNow = deriveProgressFromStep(rigState.STEP);

  if (st === "IDLE") {
    clearSegmentIssues();
  }
  const looksLikeFreshRunStart =
    (st === "RUNNING") &&
    (gPrevState === "IDLE" || gPrevState === null) &&
    (!!stepNow) &&
    (Number(stepNow.cur) <= 1);
  if (looksLikeFreshRunStart) {
    clearSegmentIssues();
  }
  if (gPrevState === "RUNNING" && st !== "IDLE" && gPrevTcpConnected && !tcpConnected) {
    markIssueAtCurrentSegment("error", "tcp disconnect");
  }
  if (gPrevState === "RUNNING" && st !== "IDLE" && gPrevBleConnected === true && !bleNow) {
    markIssueAtCurrentSegment("error", "ble disconnect");
  }

  const addKV = (k, v, dim = false) => {
    let rendered = v;
    if (k === "FLASH_GUARD_MS" || k === "FLASH_GUARD_REMAIN_MS") {
      rendered = formatFlashGuardSeconds(v);
    }
    const div = document.createElement("div");
    div.className = "kv";
    div.innerHTML = `<div class="k" title="${k}">${labelForKey(k)}</div><div class="v${dim ? " dim" : ""}">${rendered}</div>`;
    statusGrid.appendChild(div);
  };

  // Derived status (from steps)
  const rotSteps = Number(rigState.ROT_STEPS);
  const tiltSteps = Number(rigState.TILT_STEPS);

  if (Number.isFinite(rotSteps) && rotSteps > 0) {
    addKV("ROT_STEP_DEG", formatDeg(360 / rotSteps));
  }

  if (Number.isFinite(tiltSteps) && tiltSteps > 0) {
    const tiltFrom = Number(rigState.TILT_FROM);
    const tiltTo = Number(rigState.TILT_TO);
    if (Number.isFinite(tiltFrom) && Number.isFinite(tiltTo)) {
      const denom = Math.max(1, tiltSteps - 1);
      addKV("TILT_STEP_DEG", formatDeg((tiltTo - tiltFrom) / denom));
    }
  }

  const step = deriveProgressFromStep(rigState.STEP);
  const hasValidVizConfig = Number.isFinite(rotSteps) && rotSteps > 0 && Number.isFinite(tiltSteps) && tiltSteps > 0;
  let rotTargetDeg = null;
  let tiltTargetDeg = null;
  if (hasValidVizConfig) {
    if (step && Number.isFinite(step.cur)) {
      const completedNow = Math.max(0, Math.min(Number(step.cur) || 0, rotSteps * tiltSteps));
      const allDoneNow = completedNow >= (rotSteps * tiltSteps);
      const activeTiltIdxNow = Math.min(Math.max(Math.floor(completedNow / rotSteps), 0), Math.max(tiltSteps - 1, 0));
      const rotDoneNow = allDoneNow ? rotSteps : (completedNow % rotSteps);
      const rotIdxNow = Math.min(rotDoneNow, Math.max(rotSteps - 1, 0));
      rotTargetDeg = rotIdxNow * (360 / rotSteps);
      tiltTargetDeg = tiltAtIndexJS(activeTiltIdxNow, tiltSteps, rigState.TILT_FROM, rigState.TILT_TO);
    }
    addKV("ROT_TARGET_DEG", Number.isFinite(rotTargetDeg) ? formatDeg(rotTargetDeg) : "-", !Number.isFinite(rotTargetDeg));
    addKV("TILT_TARGET_DEG", Number.isFinite(tiltTargetDeg) ? formatDeg(tiltTargetDeg) : "-", !Number.isFinite(tiltTargetDeg));
  }
  if (step && hasValidVizConfig && rigState.STATE === "RUNNING") {
    const totalSteps = rotSteps * tiltSteps;
    const completed = Math.max(0, Math.min(Number(step.cur) || 0, totalSteps));
    const allDone = completed >= totalSteps;
    const subNum = Number(rigState.SUB);
    const tiltDone = allDone ? tiltSteps : Math.floor(completed / rotSteps);
    const activeTiltIdx = Math.min(Math.max(Math.floor(completed / rotSteps), 0), Math.max(tiltSteps - 1, 0));
    const tiltIdx = activeTiltIdx;
    const tiltSegCount = Math.max(1, tiltSteps - 1);
    const tiltDoneSegs = Math.max(0, Math.min(tiltSegCount, tiltDone));
    const activeTiltSeg = Math.max(0, Math.min(tiltSegCount - 1, tiltDone));
    const rotDone = allDone ? rotSteps : (completed % rotSteps);
    const rotIdx = Math.min(rotDone, Math.max(rotSteps - 1, 0));
    const rotStepDeg = 360 / rotSteps;
    const tiltTarget = tiltAtIndexJS(tiltIdx, tiltSteps, rigState.TILT_FROM, rigState.TILT_TO);

    // Top view progress (rotation within current tilt)
    const angleLast = Number(rigState.ANGLE_LAST);
    if (!gVizWasRunning || gVizTiltIdx === null || gVizTiltIdx !== tiltIdx) {
      gVizTiltIdx = tiltIdx;
      if (Number.isFinite(angleLast)) {
        const rotStepDegAnchor = 360 / rotSteps;
        let rotDoneAnchor = rotDone;
        if (!allDone && (subNum >= 8 && subNum <= 10)) rotDoneAnchor = Math.min(rotSteps, rotDoneAnchor + 1);
        gVizRotStartAngle = angleLast - (rotDoneAnchor * rotStepDegAnchor);
      } else {
        gVizRotStartAngle = null;
      }
      gVizRotStartAngleApplied = null;
    }
    let startAngle = -90;
    if (Number.isFinite(gVizRotStartAngle)) startAngle = gVizRotStartAngle - 90;
    if (gVizRotStepsBuilt !== rotSteps || !Number.isFinite(gVizRotStartAngleApplied) || Math.abs(gVizRotStartAngleApplied - startAngle) > 0.2) {
      buildRotSegments(rotSteps, startAngle);
    }
    let rotDoneVisual = rotDone;
    if (!allDone && (subNum >= 8 && subNum <= 10)) {
      rotDoneVisual = Math.min(rotSteps, rotDone + 1);
    }

    let rotDoneCommitted = rotDoneVisual;
    if (!allDone && Number.isFinite(angleLast) && Number.isFinite(gVizRotStartAngle)) {
      const stepDeg = 360 / rotSteps;
      const delta = (angleLast - gVizRotStartAngle + 360) % 360;
      const measured = Math.floor((delta + (stepDeg * 0.10)) / stepDeg);
      rotDoneCommitted = Math.max(rotDoneVisual, Math.max(0, Math.min(rotSteps, measured)));
    }
    const rotBlink = (rigState.STATE === "RUNNING") && ((subNum >= 6 && subNum <= 10) || subNum === 12) && !allDone;
    const activeRotIdx = Math.max(0, Math.min(rotSteps - 1, rotDoneCommitted));
    updateRotSegmentsDone(rotDoneCommitted, activeRotIdx, rotBlink, rotSteps);

    if (rotNeedle) {
      const fracAngle = rotSteps > 0 ? ((rotDoneCommitted / rotSteps) * 360) : 0;
      const needleAngle = Number.isFinite(angleLast) ? (angleLast - 90) : (startAngle + fracAngle);
      rotNeedle.setAttribute("transform", `rotate(${needleAngle} 60 60)`);
    }
    if (rotMeta) {
      const angTxt = Number.isFinite(angleLast) ? formatDeg(angleLast) : "-";
      rotMeta.textContent = `Rot ${Math.min(rotDoneCommitted + 1, rotSteps)}/${rotSteps} | ${angTxt} deg`;
    }

    // Side view progress (tilt steps across configured range)
    const tiltFromNum = Number(rigState.TILT_FROM);
    const tiltToNum = Number(rigState.TILT_TO);
    if (gVizTiltStepsBuilt !== tiltSteps || gVizTiltFromBuilt !== tiltFromNum || gVizTiltToBuilt !== tiltToNum) buildTiltSegments(tiltSteps, tiltFromNum, tiltToNum);
    const tiltBlink = (rigState.STATE === "RUNNING") && ((subNum >= 6 && subNum <= 10) || subNum === 12) && !allDone;
    const tiltActiveSeg = (activeTiltIdx >= (tiltSteps - 1)) ? Math.max(0, tiltSegCount - 1) : activeTiltSeg;
    updateTiltSegmentsDone(tiltDoneSegs, tiltActiveSeg, tiltBlink);
    if (tiltNeedle) {
      const tiltFrac = tiltSteps > 1 ? (tiltIdx / (tiltSteps - 1)) : 0.5;
      const angle = -90 + (tiltFrac * 180);
      tiltNeedle.setAttribute("transform", `rotate(${angle} 80 100)`);
    }
    if (tiltMeta) tiltMeta.textContent = `Tilt ${tiltIdx + 1}/${tiltSteps} | ${formatDeg(tiltTarget)} deg`;
    gVizWasRunning = (rigState.STATE === "RUNNING");
  }
  if (hasValidVizConfig && rigState.STATE === "IDLE") {
    const tiltFromNum = Number(rigState.TILT_FROM);
    const tiltToNum = Number(rigState.TILT_TO);
    const angleLast = Number(rigState.ANGLE_LAST);
    const idleStartAngle = Number.isFinite(angleLast) ? (angleLast - 90) : -90;
    const shouldRebuildRot = (
      gVizRotStepsBuilt !== rotSteps ||
      !Number.isFinite(gVizRotStartAngleApplied) ||
      Math.abs(gVizRotStartAngleApplied - idleStartAngle) > 0.2
    );
    if (shouldRebuildRot) buildRotSegments(rotSteps, idleStartAngle);
    updateRotSegmentsDone(0, 0, false, rotSteps);
    if (rotNeedle) rotNeedle.setAttribute("transform", `rotate(${idleStartAngle} 60 60)`);
    if (rotMeta) {
      const angTxt = Number.isFinite(angleLast) ? formatDeg(angleLast) : "-";
      rotMeta.textContent = `Rot 1/${rotSteps} | ${angTxt} deg`;
    }

    if (gVizTiltStepsBuilt !== tiltSteps || gVizTiltFromBuilt !== tiltFromNum || gVizTiltToBuilt !== tiltToNum) {
      buildTiltSegments(tiltSteps, tiltFromNum, tiltToNum);
    }
    const tiltSegCount = Math.max(1, tiltSteps - 1);
    updateTiltSegmentsDone(0, Math.max(0, tiltSegCount - 1), false);
    if (tiltNeedle) tiltNeedle.setAttribute("transform", "rotate(-90 80 100)");
    if (tiltMeta) tiltMeta.textContent = `Tilt 1/${tiltSteps} | ${formatDeg(tiltAtIndexJS(0, tiltSteps, tiltFromNum, tiltToNum))} deg`;

    gVizTiltIdx = 0;
    gVizRotStartAngle = Number.isFinite(angleLast) ? angleLast : null;
    gVizRotStartAngleApplied = idleStartAngle;
    gVizWasRunning = false;
  }
  const shouldClearViz = !hasValidVizConfig;
  if (shouldClearViz) {
    clearVizSegments(rotViz, "vizSeg");
    clearVizSegments(tiltViz, "vizTiltSeg");
    if (rotNeedle) rotNeedle.setAttribute("transform", "rotate(-90 60 60)");
    if (tiltNeedle) tiltNeedle.setAttribute("transform", "rotate(0 80 100)");
    if (rotMeta) rotMeta.textContent = "-";
    if (tiltMeta) tiltMeta.textContent = "-";
    gVizTiltIdx = null;
    gVizRotStartAngle = null;
    gVizRotStartAngleApplied = null;
    gVizRotStepsBuilt = 0;
    gVizRotDisplayStepsBuilt = 0;
    gVizTiltStepsBuilt = 0;
    gVizTiltFromBuilt = null;
    gVizTiltToBuilt = null;
    gVizWasRunning = false;
  }

  let tiltWaitMs = null;
  const tiltMoveMs = Number(rigState.TILT_MOVE_MS);
  const tiltReserveMs = Number(rigState.TILT_RESERVE_MS);
  if (Number.isFinite(tiltMoveMs)) {
    const waitMs = tiltMoveMs + (Number.isFinite(tiltReserveMs) ? tiltReserveMs : 0);
    tiltWaitMs = waitMs;
    addKV("TILT_WAIT_MS", `${Math.round(waitMs)}`);
  }

  const snapSettleMs = Number(rigState.SNAP_SETTLE_MS);
  if (Number.isFinite(snapSettleMs)) {
    addKV("SNAP_SETTLE_MS", `${Math.round(snapSettleMs)}`);
  }
  const snapCooldownMs = Number(rigState.SNAP_COOLDOWN_MS);
  if (Number.isFinite(snapCooldownMs)) {
    addKV("SNAP_COOLDOWN_MS", `${Math.round(snapCooldownMs)}`);
  }

  if (Number.isFinite(rotSteps) && rotSteps > 0 && Number.isFinite(tiltSteps) && tiltSteps > 0) {
    const presetKey = presetKeyForSteps(rotSteps, tiltSteps);
    const rotAvg = Number(rigState.ROT_STEP_MS_AVG);
    if (presetKey && rotAvg > 0) {
      const cur = presetTiming[presetKey] || {};
      if (rotAvg > 0) cur.rotStepMs = rotAvg;
      cur.ts = Date.now();
      presetTiming[presetKey] = cur;
      savePresetTiming();
    }

    const parts = computeTimeParts(presetKey);
    const totalShots = rotSteps * tiltSteps;
    const baseTotalMs = (tiltSteps * parts.tiltWaitMs) + (totalShots * parts.perShotMs);
    const totalGuardPauses = (parts.flashGuardEnabled && parts.flashGuardEvery > 0 && parts.flashGuardMs > 0 && totalShots > 1)
      ? Math.floor((totalShots - 1) / parts.flashGuardEvery)
      : 0;
    const totalMs = baseTotalMs + (totalGuardPauses * parts.flashGuardMs);

    let remainingMs = totalMs;
    if (st === "RUNNING" && step && step.tot > 0) {
      const nextIdx = Math.min(step.cur, step.tot - 1);
      const tiltIdx = Math.floor(nextIdx / rotSteps);
      const subNum = Number(rigState.SUB);
      const inTiltPhase = (subNum === 4 || subNum === 5);
      const remainingTilts = inTiltPhase ? (tiltSteps - tiltIdx) : Math.max(0, tiltSteps - (tiltIdx + 1));
      const remainingShots = Math.max(0, step.tot - step.cur);
      const baseRemainingMs = (remainingTilts * parts.tiltWaitMs) + (remainingShots * parts.perShotMs);
      let remainingGuardMs = 0;
      if (parts.flashGuardEnabled && parts.flashGuardEvery > 0 && parts.flashGuardMs > 0 && step.tot > 1) {
        const totalPauses = Math.floor((step.tot - 1) / parts.flashGuardEvery);
        const passedPauses = Math.floor(step.cur / parts.flashGuardEvery);
        const futurePauses = Math.max(0, totalPauses - passedPauses);
        remainingGuardMs = futurePauses * parts.flashGuardMs;
        if (subNum === 12) {
          const currentRemain = Number(rigState.FLASH_GUARD_REMAIN_MS);
          if (Number.isFinite(currentRemain) && currentRemain > 0) {
            remainingGuardMs += currentRemain;
          }
        }
      }
      remainingMs = baseRemainingMs + remainingGuardMs;
    }

    if (seqTimePill) {
      if (st === "RUNNING") {
        seqTimePill.textContent = remainingMs > 0 ? `Time Left: ${formatDuration(remainingMs)}` : "Time Left: -";
        seqTimePill.title = "Live estimate (auto-updates during scan)";
      } else {
        seqTimePill.textContent = totalMs > 0 ? `Time: ${formatDuration(totalMs)}` : "Time: -";
        seqTimePill.title = "Estimate based on current settings";
      }
    }
  } else if (seqTimePill) {
    seqTimePill.textContent = "Time: -";
    seqTimePill.title = "";
  }

  const preferred = [
    "AF_MODE",
    "AF_PREFOCUS_MS",
    "AF_SHUTTER_MS",
    "AF_POSTFOCUS_MS",
    "SNAP_PRESS_MS",
    "MANUAL_PREFOCUS_MS",
    "FLASH_GUARD",
    "FLASH_GUARD_EVERY",
    "FLASH_GUARD_MS",
    "FLASH_GUARD_REMAIN_MS",
    "ROT_STEP_DEG",
    "TILT_STEP_DEG",
    "ROT_TARGET_DEG",
    "TILT_TARGET_DEG",
    "TILT_WAIT_MS",
    "SNAP_SETTLE_MS",
    "SNAP_COOLDOWN_MS",
    "ROT_ANGLE",
    "TILT_ANGLE",
    "STEP",
    "ROT_STEPS",
    "TILT_STEPS",
    "TILT_FROM",
    "TILT_TO",
  ];
  const keys = Object.keys(rigState).filter(k => !HIDDEN_STATUS_KEYS.has(k));
  const used = new Set();
  for (const k of preferred) {
    if (k in rigState && !HIDDEN_STATUS_KEYS.has(k)) {
      addKV(k, rigState[k]);
      used.add(k);
    }
  }
  keys
    .filter(k => !used.has(k))
    .sort((a,b) => a.localeCompare(b))
    .forEach(k => addKV(k, rigState[k]));

  setBadge(tcpBadge, `TCP: ${tcpConnected ? "CONNECTED" : "DISCONNECTED"}`, tcpConnected);
  const ip = rigState.IP || "-";
  setBadge(ipBadge, `IP: ${ip}:${rigState.PORT || "-"}`, !!rigState.IP);
  const fwVer = String(rigState.FW_VER || "-");
  const uiVer = String(rigState.UI_VER || "-");
  const buildGit = String(rigState.BUILD_GIT || "-");
  const buildTime = String(rigState.BUILD_TIME || "-");
  if (appFooterText) {
    appFooterText.textContent = `(c) 2026 Superwutz / RealityScanRig3000 | FW ${fwVer} | UI ${uiVer} | Build ${buildGit} | ${buildTime}`;
  }
  if (manifestUrlInput && !manifestUrlInput.value) {
    const stored = (() => { try { return localStorage.getItem("scanrig.update.manifestUrl") || ""; } catch { return ""; } })();
    manifestUrlInput.value = stored || DEFAULT_MANIFEST_URL;
  }
  if (!latestManifestVersion) {
    setFwVersionInfo(`Version: FW ${fwVer} / UI ${uiVer}`);
  } else {
    const cmp = compareVersions(latestManifestVersion, fwVer);
    if (cmp > 0) setFwVersionInfo(`Update available: ${latestManifestVersion} (current ${fwVer})`);
    else setFwVersionInfo(`Up to date: ${fwVer} (latest ${latestManifestVersion})`);
  }


  statePill.textContent = `STATE: ${st}`;
  if (actionText) {
    let msg = "Idle";
    if (!tcpConnected) {
      msg = "Disconnected";
    } else if (st === "PAUSED") {
      msg = "Paused";
    } else if (st === "IDLE") {
      msg = "Idle";
    } else if (st === "RUNNING") {
      const subNum = Number(rigState.SUB);
      if (subNum === 4 || subNum === 5) {
        const wait = Number.isFinite(tiltWaitMs) ? ` (~${formatSeconds(tiltWaitMs)})` : "";
        msg = `Tilting${wait}`;
      } else if (subNum === 6 || subNum === 7) {
        msg = "Rotating";
      } else if (subNum === 8) {
        msg = "Settling";
      } else if (subNum === 9) {
        msg = "Triggering camera";
      } else if (subNum === 10) {
        msg = "Cooldown";
      } else if (subNum === 12) {
        const remMs = Number(rigState.FLASH_GUARD_REMAIN_MS);
        msg = Number.isFinite(remMs) && remMs > 0
          ? `Flash cooldown (${formatDuration(remMs)})`
          : "Flash cooldown";
      } else if (subNum === 1 || subNum === 2 || subNum === 3) {
        msg = "Recovering";
      } else if (subNum === 11) {
        msg = "Done";
      } else if (Number.isFinite(subNum) && SUB_LABELS[subNum]) {
        msg = SUB_LABELS[subNum];
      } else {
        msg = "Running";
      }
    } else {
      msg = "Running";
    }
    actionText.textContent = msg;
  }
  if (actionRow) {
    actionRow.classList.remove("paused", "idle", "disconnected");
    if (!tcpConnected) actionRow.classList.add("disconnected");
    else if (st === "PAUSED") actionRow.classList.add("paused");
    else if (st === "IDLE") actionRow.classList.add("idle");
  }

  const shots = computeShots(rigState.ROT_STEPS, rigState.TILT_STEPS);
  if (shotsPill) shotsPill.textContent = shots ? `Shots: ${shots.total}` : "Shots: -";
  const prog = deriveProgressFromStep(rigState.STEP);
  const completedNow = !!(gPrevState === "RUNNING" && st === "IDLE" && prog && prog.tot > 0 && prog.cur >= prog.tot);
  if (completedNow && !victoryVisible) {
    showVictory(`Sequence finished: ${prog.cur}/${prog.tot} shots captured.`);
    addLog(`[SEQ] complete (${prog.cur}/${prog.tot})`);
  }
  if (st === "RUNNING") {
    if (startWarn) startWarn.classList.add("hidden");
    if (victoryVisible) hideVictory();
  }
  const rotStepsNum = Number(rigState.ROT_STEPS);
  const tiltStepsNum = Number(rigState.TILT_STEPS);
  const seqTotal = (Number.isFinite(rotStepsNum) && rotStepsNum > 0 && Number.isFinite(tiltStepsNum) && tiltStepsNum > 0)
    ? (rotStepsNum * tiltStepsNum)
    : 0;
    const forceResetProgress = (rigState.STATE === "IDLE");
  if (!forceResetProgress && prog) {
    progressText.textContent = `${prog.cur}/${prog.tot} (${Math.round(prog.frac * 100)}%)`;
  } else if (seqTotal > 0) {
    progressText.textContent = `0/${seqTotal} (0%)`;
  } else {
    progressText.textContent = "-";
  }

  if (seqTotal > 0) {
    if (gSeqRotStepsBuilt !== rotStepsNum || gSeqTiltStepsBuilt !== tiltStepsNum || gSeqNodes.length === 0) {
      buildSequenceSegments(rotStepsNum, tiltStepsNum);
    }
    const done = (!forceResetProgress && prog) ? Math.max(0, Math.min(seqTotal, Number(prog.cur) || 0)) : 0;
    const subNum = Number(rigState.SUB);
    const blink = (!forceResetProgress) && (rigState.STATE === "RUNNING") && ((subNum >= 4 && subNum <= 10) || subNum === 12) && done < seqTotal;
    const activeIdx = (rigState.STATE === "RUNNING" && done < seqTotal) ? done : -1;
    updateSequenceSegments(done, activeIdx, blink);
  } else {
    clearSequenceSegments();
  }

  renderPresetActive();
  renderPresetEffective();
  renderPresetSelection();
  updateTriggerToggle();
updateAutoApplyToggle();
  updateTurntableBadge();
  updateTriggerBackendUi();
  populateQuickInputs();

  const disabled = !tcpConnected;
  [
    "btnStart","btnPause","btnAbort",
    "btnTtUp","btnTtTiltZero","btnTtDown",
    "btnTtLeft","btnTtRotZero","btnTtRight","btnSnap","btnTtStop",
    "triggerModeSelect","btnAutofocusToggle","btnFlashGuardToggle","btnPhonePairToggle","btnPhoneDisconnect",
    "btnSet","btnApplyPreset","btnFwUpload",
    "btnSetRotSteps","btnSetTiltSteps",
    "btnSetTiltFrom","btnSetTiltTo",
    "btnSetSnapSettle","btnSetSnapCooldown",
    "btnSetFlashGuardEvery","btnSetFlashGuardMs",
    "btnTestFocus","btnTestTrigger"
  ].forEach(id => { const b = el(id); if (b) b.disabled = disabled; });

  const startBtn = el("btnStart");
  if (startBtn) {
    startBtn.textContent = (rigState.STATE === "PAUSED") ? "RESUME" : "START";
  }

  const running = (rigState.STATE === "RUNNING");
  const lockMsg = running ? "Locked while scan is running" : "";
  const setLock = (btn) => {
    if (!btn) return;
    btn.disabled = disabled || running;
    btn.title = running ? lockMsg : "";
  };
  setLock(btnApplyPreset);
  setLock(btnSetRotSteps);
  setLock(btnSetTiltSteps);
  setLock(btnSetTiltFrom);
  setLock(btnSetTiltTo);
  setLock(btnTtUp);
  setLock(btnTtTiltZero);
  setLock(btnTtDown);
  setLock(btnTtLeft);
  setLock(btnTtRotZero);
  setLock(btnSnap);
  setLock(btnTtStop);
  setLock(btnTtRight);
  setLock(triggerModeSelect);
  setLock(btnAutofocusToggle);
  setLock(btnFlashGuardToggle);
  setLock(btnSetFlashGuardEvery);
  setLock(btnSetFlashGuardMs);
  setLock(btnPhonePairToggle);
  setLock(btnPhoneDisconnect);
  setLock(btnFwUpload);
  if (manualControls) manualControls.classList.toggle("locked", running);
  if (manualLockOverlay) manualLockOverlay.title = running ? lockMsg : "";
  if (inpRotSteps) inpRotSteps.title = running ? lockMsg : "";
  if (inpTiltSteps) inpTiltSteps.title = running ? lockMsg : "";
  if (inpTiltFrom) inpTiltFrom.title = running ? lockMsg : "";
  if (inpTiltTo) inpTiltTo.title = running ? lockMsg : "";
  if (inpFlashGuardEvery) inpFlashGuardEvery.disabled = disabled || running;
  if (inpFlashGuardMs) inpFlashGuardMs.disabled = disabled || running;
  if (inpFlashGuardEvery) inpFlashGuardEvery.title = running ? lockMsg : "";
  if (inpFlashGuardMs) inpFlashGuardMs.title = running ? lockMsg : "";

  gPrevState = st;
  gPrevTcpConnected = tcpConnected;
  gPrevBleConnected = bleNow;
}

function connectWs() {
  const url = (location.protocol === "https:" ? "wss://" : "ws://") + location.host;
  ws = new WebSocket(url);

  ws.onopen = () => {
    setBadge(connBadge, "WS: CONNECTED", true);
    addLog(`[UI] WS connected`);
    ws.send(JSON.stringify({ type: "requestStatus" }));
  };

  ws.onclose = () => {
    if (rigState.STATE === "RUNNING") {
      markIssueAtCurrentSegment("error", "ws disconnect");
    }
    setBadge(connBadge, "WS: DISCONNECTED", false);
    addLog(`[UI] WS disconnected`);
    tcpConnected = false;
    renderStatus();
    setTimeout(connectWs, 800);
  };

  ws.onmessage = (ev) => {
    let msg;
    try { msg = JSON.parse(ev.data); } catch { return; }

    if (msg.type === "hello") {
      rigState = msg.rigState || {};
      window.__rigState = rigState;
      tcpConnected = !!msg.tcpConnected;
      renderStatus();
      return;
    }

    if (msg.type === "log") {
      addLog(msg.msg);
      const m = String(msg.msg || "");
      if (rigState.STATE !== "IDLE") detectAndMarkIssueFromText(m);
      if (m.includes("[TT]")) setManualLastAction(`${nowClockText()} ${m}`);
      return;
    }
    if (msg.type === "rigLine") {
      addLog(msg.line);
      if (rigState.STATE !== "IDLE") detectAndMarkIssueFromText(msg.line);
      return;
    }

    if (msg.type === "status") {
      if (msg.rigState) rigState = msg.rigState;
      window.__rigState = rigState;
      if (typeof msg.tcpConnected === "boolean") tcpConnected = msg.tcpConnected;
      renderStatus();
      return;
    }
  };
}

// UI events
tabControl && (tabControl.onclick = () => setView("control"));
tabSettings && (tabSettings.onclick = () => setView("settings"));
setUseStatic && (setUseStatic.onchange = () => { toggleStaticFields(); updateSettingsDirtyState(); });
setLedBrightness && (setLedBrightness.oninput = () => syncLedBrightnessLabel());
btnNetReload && (btnNetReload.onclick = () => loadNetworkSettings());
btnNetSave && (btnNetSave.onclick = () => saveNetworkSettings(false));
btnNetSaveReboot && (btnNetSaveReboot.onclick = () => saveNetworkSettings(true));
[
  setWifiSsid, setWifiPass, setIp, setGw, setDns, setMask, setLedEnable
].forEach((node) => {
  if (!node) return;
  node.addEventListener("input", () => updateSettingsDirtyState());
  node.addEventListener("change", () => updateSettingsDirtyState());
});
btnTestFocus && (btnTestFocus.onclick = () => {
  sendCmd("SNAP_FOCUS");
  setManualLastAction(`${nowClockText()} focus relay test requested`);
});
btnTestTrigger && (btnTestTrigger.onclick = () => {
  sendCmd("SNAP_TRIGGER");
  setManualLastAction(`${nowClockText()} trigger relay test requested`);
});
btnVictoryClose && (btnVictoryClose.onclick = () => hideVictory());
btnVictoryReset && (btnVictoryReset.onclick = () => {
  hideVictory();
  sendCmd("RESET");
  sendCmd("STATUS");
});

btnAutoApplyToggle && (btnAutoApplyToggle.onclick = () => {
  autoApplyEnabled = !autoApplyEnabled;
  updateAutoApplyToggle();
  updateTurntableBadge();
  addLog(`[UI] Auto-Apply ${autoApplyEnabled ? "ENABLED" : "DISABLED"}`);
});


btnTriggerToggle && (btnTriggerToggle.onclick = () => {
  triggerEnabled = !triggerEnabled;
  updateTriggerToggle();
  addLog(`[UI] Trigger ${triggerEnabled ? "ENABLED" : "DISABLED"} (dry-run)`);
});

triggerModeSelect && (triggerModeSelect.onchange = () => {
  if (String(rigState.STATE || "") === "RUNNING") return;
  const mode = String(triggerModeSelect.value || "HARDWARE").toUpperCase();
  if (mode === "SMARTPHONE") sendCmd("TRIGGER_MODE_SMARTPHONE");
  else sendCmd("TRIGGER_MODE_HW");
  sendCmd("STATUS");
});

btnAutofocusToggle && (btnAutofocusToggle.onclick = () => {
  if (String(rigState.STATE || "") === "RUNNING") return;
  const afMode = String(rigState.AF_MODE || "AUTO").toUpperCase();
  const afAuto = (afMode !== "MANUAL");
  sendCmd(afAuto ? "AF_MODE_MANUAL" : "AF_MODE_AUTO");
  setManualLastAction(`${nowClockText()} autofocus mode ${afAuto ? "manual" : "auto"} requested`);
});

btnFlashGuardToggle && (btnFlashGuardToggle.onclick = () => {
  if (String(rigState.STATE || "") === "RUNNING") return;
  const guardRaw = String(rigState.FLASH_GUARD ?? "1").toLowerCase();
  const guardOn = !(guardRaw === "0" || guardRaw === "false");
  sendCmd(guardOn ? "FLASH_GUARD_OFF" : "FLASH_GUARD_ON");
  setManualLastAction(`${nowClockText()} flash guard ${guardOn ? "off" : "on"} requested`);
});

btnSetFlashGuardEvery && (btnSetFlashGuardEvery.onclick = () => {
  if (String(rigState.STATE || "") === "RUNNING") return;
  const v = inpFlashGuardEvery ? inpFlashGuardEvery.value.trim() : "";
  if (!v) return;
  const n = Number(v);
  if (!Number.isFinite(n) || n < 1) return;
  sendCmd(`SET FLASH_GUARD_EVERY=${Math.round(n)}`);
  if (inpFlashGuardEvery) inpFlashGuardEvery.value = "";
});

btnSetFlashGuardMs && (btnSetFlashGuardMs.onclick = () => {
  if (String(rigState.STATE || "") === "RUNNING") return;
  const v = inpFlashGuardMs ? inpFlashGuardMs.value.trim() : "";
  if (!v) return;
  const n = Number(v);
  if (!Number.isFinite(n) || n < 1) return;
  sendCmd(`SET FLASH_GUARD_MS=${Math.round(n * 1000)}`);
  if (inpFlashGuardMs) inpFlashGuardMs.value = "";
});

btnPhonePairToggle && (btnPhonePairToggle.onclick = () => {
  const pairingRaw = String(rigState.PHONE_PAIRING ?? "0").toLowerCase();
  const pairing = (pairingRaw === "1" || pairingRaw === "true");
  sendCmd(pairing ? "PHONE_PAIR_STOP" : "PHONE_PAIR_START");
  setManualLastAction(`${nowClockText()} phone pairing ${pairing ? "stop" : "start"} requested`);
});

btnPhoneDisconnect && (btnPhoneDisconnect.onclick = () => {
  sendCmd("PHONE_DISCONNECT");
  setManualLastAction(`${nowClockText()} phone disconnect requested`);
});

presetButtons.quick && (presetButtons.quick.onclick = () => selectPreset("quick"));
presetButtons.medium && (presetButtons.medium.onclick = () => selectPreset("medium"));
presetButtons.detailed && (presetButtons.detailed.onclick = () => selectPreset("detailed"));

btnApplyPreset && (btnApplyPreset.onclick = () => applySelectedPreset());

const btnStart = el("btnStart");
if (btnStart) btnStart.onclick = () => {
  if (rigState.STATE === "PAUSED") {
    sendCmd("RESUME");
    return;
  }
  if (!isTurntableConnected()) {
    showStartWarning("Turntable not connected. Connect TT before starting a scan.");
    addLog("[SEQ] start blocked: turntable not connected");
    return;
  }
  if (victoryVisible) hideVictory();
  sendCmd("START");
};
const btnPause = el("btnPause");
if (btnPause) btnPause.onclick = () => sendCmd("PAUSE");
const btnAbort = el("btnAbort");
if (btnAbort) btnAbort.onclick = () => {
  sendCmd("ABORT");
  setManualLastAction(`${nowClockText()} ABORT requested`);
};
if (btnTtUp) btnTtUp.onclick = () => { sendCmd("TT_TILT_UP"); setManualLastAction(`${nowClockText()} tilt up requested`); };
if (btnTtTiltZero) btnTtTiltZero.onclick = () => {
  sendCmd("TT_TILT_ZERO");
  setManualLastAction(`${nowClockText()} tilt zero requested`);
};
if (btnTtDown) btnTtDown.onclick = () => { sendCmd("TT_TILT_DOWN"); setManualLastAction(`${nowClockText()} tilt down requested`); };
if (btnTtLeft) btnTtLeft.onclick = () => { sendCmd("TT_LEFT"); setManualLastAction(`${nowClockText()} rot left requested`); };
if (btnTtRotZero) btnTtRotZero.onclick = () => {
  sendCmd("TT_ROT_ZERO");
  setManualLastAction(`${nowClockText()} rot zero requested`);
};
if (btnTtStop) btnTtStop.onclick = () => { sendCmd("TT_STOP"); setManualLastAction(`${nowClockText()} stop requested`); };
if (btnTtRight) btnTtRight.onclick = () => { sendCmd("TT_RIGHT"); setManualLastAction(`${nowClockText()} rot right requested`); };
if (btnSnap) btnSnap.onclick = () => { sendCmd("SNAP"); setManualLastAction(`${nowClockText()} SNAP requested`); };
if (btnFwUpload) btnFwUpload.onclick = () => {
  if (!fwFile || !fwFile.files || !fwFile.files.length) {
    setFwUploadState("Select a firmware .bin first");
    return;
  }
  const file = fwFile.files[0];
  if (!String(file.name || "").toLowerCase().endsWith(".bin")) {
    setFwUploadState("Please choose a .bin file");
    return;
  }

  setFwUploadState("Preparing upload...");
  addLog("[FW] OTA upload started");
  uploadFirmwareBlob(file, file.name || "firmware.bin").catch(() => {});
};
if (btnCheckUpdate) btnCheckUpdate.onclick = async () => {
  const explicit = manifestUrlInput ? String(manifestUrlInput.value || "").trim() : "";
  const fallback = DEFAULT_MANIFEST_URL;
  const url = explicit || fallback;
  if (!url) {
    setFwVersionInfo("Set manifest URL first");
    return;
  }
  try { localStorage.setItem("scanrig.update.manifestUrl", url); } catch {}
  btnCheckUpdate.disabled = true;
  latestManifestCheckUrl = url;
  latestFirmwareUrl = "";
  if (btnFwUpdateNow) btnFwUpdateNow.disabled = true;
  setFwVersionInfo("Checking...");
  try {
    const res = await fetch(url, { cache: "no-store" });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const manifest = await res.json();
    const latest = String(manifest.version || "").trim();
    if (!latest) throw new Error("manifest has no version");
    const fwUrl = firmwareUrlFromManifest(manifest, url);
    if (!fwUrl) throw new Error("manifest has no firmware part");
    latestManifestVersion = latest;
    latestFirmwareUrl = fwUrl;
    if (btnFwUpdateNow) btnFwUpdateNow.disabled = false;
    const fwVer = String(rigState.FW_VER || "-");
    const cmp = compareVersions(latest, fwVer);
    if (cmp > 0) {
      setFwVersionInfo(`Update available: ${latest} (current ${fwVer})`);
      addLog(`[FW] update available at ${url}: ${latest} > ${fwVer}`);
    } else {
      setFwVersionInfo(`Up to date: ${fwVer} (latest ${latest})`);
      addLog(`[FW] up to date (${fwVer}) via ${url}`);
    }
  } catch (err) {
    latestManifestVersion = "";
    latestFirmwareUrl = "";
    if (btnFwUpdateNow) btnFwUpdateNow.disabled = true;
    setFwVersionInfo(`Check failed: ${String(err && err.message ? err.message : err)}`);
    addLog(`[FW] update check failed (${latestManifestCheckUrl})`);
  } finally {
    btnCheckUpdate.disabled = false;
  }
};
if (btnFwUpdateNow) btnFwUpdateNow.onclick = async () => {
  if (!latestFirmwareUrl) {
    setFwVersionInfo("Run Check Updates first");
    return;
  }
  try {
    setFwUploadState("Downloading firmware...");
    addLog(`[FW] downloading ${latestFirmwareUrl}`);
    const res = await fetch(latestFirmwareUrl, { cache: "no-store" });
    if (!res.ok) throw new Error(`download failed (HTTP ${res.status})`);
    const blob = await res.blob();
    addLog("[FW] download complete, starting OTA upload");
    await uploadFirmwareBlob(blob, "firmware.bin");
  } catch (err) {
    setFwUploadState(`Update failed: ${String(err && err.message ? err.message : err)}`);
    addLog("[FW] update failed");
  }
};

el("btnSet").onclick = () => {
  const k = el("paramKey").value.trim();
  const v = el("paramVal").value.trim();
  if (!k || !v) return;
  if (!canEditParamKey(k)) return;
  sendCmd(`SET ${k}=${v}`);
  sendCmd("STATUS");
};

if (manifestUrlInput && !manifestUrlInput.value) {
  const storedManifestUrl = (() => { try { return localStorage.getItem("scanrig.update.manifestUrl") || ""; } catch { return ""; } })();
  manifestUrlInput.value = storedManifestUrl || DEFAULT_MANIFEST_URL;
}

btnSetRotSteps && (btnSetRotSteps.onclick = () => {
  const v = inpRotSteps ? inpRotSteps.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("ROT_STEPS")) return;
  sendCmd(`SET ROT_STEPS=${v}`);
  sendCmd("STATUS");
});

btnSetTiltSteps && (btnSetTiltSteps.onclick = () => {
  const v = inpTiltSteps ? inpTiltSteps.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("TILT_STEPS")) return;
  sendCmd(`SET TILT_STEPS=${v}`);
  sendCmd("STATUS");
});

btnSetTiltFrom && (btnSetTiltFrom.onclick = () => {
  const v = inpTiltFrom ? inpTiltFrom.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("TILT_FROM")) return;
  sendCmd(`SET TILT_FROM=${v}`);
  sendCmd("STATUS");
});

btnSetTiltTo && (btnSetTiltTo.onclick = () => {
  const v = inpTiltTo ? inpTiltTo.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("TILT_TO")) return;
  sendCmd(`SET TILT_TO=${v}`);
  sendCmd("STATUS");
});

btnSetSnapSettle && (btnSetSnapSettle.onclick = () => {
  const v = inpSnapSettle ? inpSnapSettle.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("SNAP_SETTLE_MS")) return;
  sendCmd(`SET SNAP_SETTLE_MS=${v}`);
  sendCmd("STATUS");
});

btnSetSnapCooldown && (btnSetSnapCooldown.onclick = () => {
  const v = inpSnapCooldown ? inpSnapCooldown.value.trim() : "";
  if (!v) return;
  if (!canEditParamKey("SNAP_COOLDOWN_MS")) return;
  sendCmd(`SET SNAP_COOLDOWN_MS=${v}`);
  sendCmd("STATUS");
});

try {
  const last = localStorage.getItem("scanrig.manual.lastAction");
  setManualLastAction(last || "-");
} catch {
  setManualLastAction("-");
}

renderPresetEffective();
updateTriggerToggle();
toggleStaticFields();
updateSettingsDirtyState(true);
setView("control");
connectWs();

// Ensure UI stays in sync even if push updates are missed
setInterval(() => {
  if (ws && ws.readyState === 1) {
    ws.send(JSON.stringify({ type: "requestStatus" }));
  }
}, 1000);
