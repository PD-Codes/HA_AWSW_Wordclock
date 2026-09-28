// HTTP API (compatible with the AWSW V5 JSON API used by the Home Assistant integration)
// plus the web UI and browser firmware upload.

#include "web.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include "settings.h"
#include "display.h"
#include "web_ui.h"

static WebServer server(80);
static bool restartPending = false;
static uint32_t restartAt = 0;

static void sendJson(JsonDocument& doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", out);
}

static void sendResult(bool ok, const String& message, bool restart = false) {
  JsonDocument doc;
  doc["ok"] = ok;
  doc["restart"] = restart;
  doc["message"] = message;
  sendJson(doc, ok ? 200 : 400);
}

static void scheduleRestart(uint32_t delayMs = 800) {
  restartPending = true;
  restartAt = millis() + delayMs;
}

void applyTimeConfig() {
  configTzTime(cfg.timeZone.c_str(), cfg.timeServer.c_str(), "time.google.com");
}

// ---------- /api/status ----------
static void handleStatus() {
  JsonDocument doc;
  tm t;
  bool haveTime = getLocalTime(&t, 0) && t.tm_year > 120;
  char timeBuf[6] = "--:--";
  if (haveTime) strftime(timeBuf, sizeof(timeBuf), "%H:%M", &t);

  doc["version"] = FW_VERSION;
  doc["firmware"] = "WordClock Custom";
  doc["onlineMode"] = true;
  doc["time"] = timeBuf;
  doc["intensity"] = displayIntensity();
  doc["intensityLimit"] = INTENSITY_LIMIT;
  doc["maxPower"] = cfg.maxPower;
  doc["nightStatus"] = displayNightStatus();
  doc["isNight"] = displayIsNight();

  doc["timeColor"] = colorToHex(cfg.timeColor);
  doc["backColor"] = colorToHex(cfg.backColor);
  doc["timeColorNight"] = colorToHex(cfg.timeColorNight);
  doc["backColorNight"] = colorToHex(cfg.backColorNight);
  doc["tickerColor"] = colorToHex(cfg.tickerColor);
  doc["tickerBrightness"] = cfg.tickerBrightness;
  doc["timeBrightnessDay"] = cfg.timeBrightnessDay;
  doc["backBrightnessDay"] = cfg.backBrightnessDay;
  doc["timeBrightnessNight"] = cfg.timeBrightnessNight;
  doc["backBrightnessNight"] = cfg.backBrightnessNight;
  doc["brightnessDay"] = cfg.timeBrightnessDay;
  doc["brightnessNight"] = cfg.timeBrightnessNight;

  doc["nightMode"] = cfg.nightMode;
  doc["dayStart"] = minutesToHHMM(cfg.dayStart);
  doc["dayStop"] = minutesToHHMM(cfg.dayStop);
  doc["singleMinutes"] = cfg.singleMinutes;
  doc["showItIs"] = cfg.showItIs;
  doc["smoothTransition"] = cfg.smoothTransition;
  doc["digitalHourChime"] = cfg.digitalHourChime;
  doc["randomDayColors"] = cfg.randomDayColors;
  doc["startupAnimation"] = cfg.startupAnimation;
  doc["showIp"] = cfg.showIp;

  doc["origin"] = cfg.origin;
  doc["serpentine"] = cfg.serpentine;
  doc["vertical"] = cfg.vertical;

  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["hostname"] = cfg.hostname;
  doc["mac"] = WiFi.macAddress();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["uptime"] = millis() / 1000;

  doc["timeServer"] = cfg.timeServer;
  doc["timeZone"] = cfg.timeZone;
  doc["ntpOk"] = displayTimeValid();
  doc["ntpStatus"] = displayTimeValid() ? "Zeit synchronisiert" : "Warte auf Zeitserver";
  doc["language"] = 0;
  doc["languageName"] = "Deutsch";
  doc["mode"] = (int)displayMode();

  JsonArray words = doc["extraWords"].to<JsonArray>();
  for (uint8_t i = 0; i < EXTRA_WORD_COUNT; i++) {
    JsonObject w = words.add<JsonObject>();
    w["id"] = EXTRA_WORDS[i].id;
    w["name"] = EXTRA_WORDS[i].name;
    w["active"] = cfg.words[i].active;
    w["color"] = colorToHex(cfg.words[i].color);
  }
  sendJson(doc);
}

// ---------- /api/set ----------
static bool parseBool(const String& v) {
  return v == "1" || v.equalsIgnoreCase("true") || v.equalsIgnoreCase("on");
}

static bool setLevel(const String& v, uint8_t& target, int maxValue = INTENSITY_LIMIT) {
  if (v.length() == 0) return false;
  target = constrain(v.toInt(), 0, maxValue);
  return true;
}

static void handleSet() {
  String ignored;
  String errors;
  bool timeChanged = false;
  bool hostnameChanged = false;

  for (int i = 0; i < server.args(); i++) {
    String k = server.argName(i);
    String v = server.arg(i);
    bool ok = true;

    if (k == "timeColor") ok = hexToColor(v, cfg.timeColor);
    else if (k == "backColor") ok = hexToColor(v, cfg.backColor);
    else if (k == "timeColorNight") ok = hexToColor(v, cfg.timeColorNight);
    else if (k == "backColorNight") ok = hexToColor(v, cfg.backColorNight);
    else if (k == "tickerColor") ok = hexToColor(v, cfg.tickerColor);
    else if (k == "timeBrightnessDay" || k == "brightnessDay") ok = setLevel(v, cfg.timeBrightnessDay);
    else if (k == "timeBrightnessNight" || k == "brightnessNight") ok = setLevel(v, cfg.timeBrightnessNight);
    else if (k == "backBrightnessDay") ok = setLevel(v, cfg.backBrightnessDay);
    else if (k == "backBrightnessNight") ok = setLevel(v, cfg.backBrightnessNight);
    else if (k == "tickerBrightness") ok = setLevel(v, cfg.tickerBrightness);
    else if (k == "maxPower") { ok = setLevel(v, cfg.maxPower, 100); if (cfg.maxPower < 5) cfg.maxPower = 5; }
    else if (k == "nightMode") cfg.nightMode = parseBool(v);
    else if (k == "dayStart") ok = hhmmToMinutes(v, cfg.dayStart);
    else if (k == "dayStop") ok = hhmmToMinutes(v, cfg.dayStop);
    else if (k == "singleMinutes") cfg.singleMinutes = parseBool(v);
    else if (k == "showItIs") cfg.showItIs = parseBool(v);
    else if (k == "smoothTransition") cfg.smoothTransition = parseBool(v);
    else if (k == "digitalHourChime") cfg.digitalHourChime = parseBool(v);
    else if (k == "randomDayColors") cfg.randomDayColors = parseBool(v);
    else if (k == "startupAnimation") cfg.startupAnimation = parseBool(v);
    else if (k == "showIp") cfg.showIp = parseBool(v);
    else if (k == "origin") cfg.origin = constrain(v.toInt(), 0, 3);
    else if (k == "serpentine") cfg.serpentine = parseBool(v);
    else if (k == "vertical") cfg.vertical = parseBool(v);
    else if (k == "timeZone") { cfg.timeZone = v; timeChanged = true; }
    else if (k == "timeServer") { cfg.timeServer = v; timeChanged = true; }
    else if (k == "hostname") {
      v.trim();
      ok = v.length() > 0 && v.length() <= 32;
      if (ok) { cfg.hostname = v; hostnameChanged = true; }
    }
    else if (k.startsWith("ewColor")) {
      int idx = extraWordIndex(k.substring(7).toInt());
      ok = idx >= 0 && hexToColor(v, cfg.words[idx].color);
    }
    else if (k.startsWith("ew")) {
      int idx = extraWordIndex(k.substring(2).toInt());
      ok = idx >= 0;
      if (ok) cfg.words[idx].active = (v == "toggle") ? !cfg.words[idx].active : parseBool(v);
    }
    else if (k == "uiTheme" || k == "board") { /* UI-only keys of the original firmware */ }
    else { ignored += (ignored.length() ? ", " : "") + k; continue; }

    if (!ok) errors += (errors.length() ? ", " : "") + k;
  }

  settingsSave();
  if (timeChanged) applyTimeConfig();
  displayRefresh();

  if (errors.length()) { sendResult(false, "Ungültiger Wert: " + errors); return; }
  String msg = "Gespeichert";
  if (ignored.length()) msg += " (ignoriert: " + ignored + ")";
  if (hostnameChanged) msg += " - Hostname gilt nach Neustart";
  sendResult(true, msg);
}

// ---------- /api/action ----------
static void handleAction() {
  String cmd = server.arg("cmd");
  if (cmd == "restart") {
    sendResult(true, "Neustart...", true);
    displayStatusWord(10, "NEUSTART");
    scheduleRestart();
  } else if (cmd == "test") {
    displaySetMode(MODE_LEDTEST);
    sendResult(true, "LED-Test gestartet");
  } else if (cmd == "digitalTimeTest") {
    displayDigital(5000);
    sendResult(true, "Digitale Uhrzeit");
  } else if (cmd == "resetExtraWords") {
    for (uint8_t i = 0; i < EXTRA_WORD_COUNT; i++) cfg.words[i].active = false;
    settingsSave();
    sendResult(true, "Alle Extra-Wörter aus");
  } else if (cmd == "wordReset") {
    resetExtraWords();
    settingsSave();
    sendResult(true, "Wortfarben zurückgesetzt");
  } else if (cmd == "calibrate") {
    displaySetMode(MODE_CALIBRATE);
    sendResult(true, "Kalibriermuster");
  } else if (cmd == "chase") {
    displaySetMode(MODE_CHASE);
    sendResult(true, "Lauflicht-Test");
  } else if (cmd == "wordCycle") {
    displaySetMode(MODE_WORDCYCLE);
    sendResult(true, "Extra-Wörter werden durchlaufen");
  } else if (cmd == "allOn") {
    displaySetMode(MODE_ALLON);
    sendResult(true, "Alle LEDs an");
  } else if (cmd == "stop") {
    displaySetMode(MODE_CLOCK);
    sendResult(true, "Zurück zur Uhr");
  } else if (cmd == "wifiReset") {
    sendResult(true, "WLAN gelöscht, Neustart in WPS-/Einrichtungsmodus", true);
    WiFiManager wm;
    wm.resetSettings();
    displayStatusWord(3, "WLAN");
    scheduleRestart();
  } else {
    sendResult(false, "Unbekannter Befehl: " + cmd);
  }
  displayRefresh();
}

// ---------- /api/ticker, /api/time, /api/preview ----------
static void handleTicker() {
  String text = server.arg("text");
  if (text.length() == 0) { sendResult(false, "Text fehlt"); return; }
  if (text.length() > 200) text = text.substring(0, 200);
  uint32_t color;
  if (server.hasArg("color") && hexToColor(server.arg("color"), color)) displayTicker(text, color);
  else displayTicker(text);
  sendResult(true, "Lauftext gestartet");
}

// Accepts ISO 8601 local time, e.g. 2026-09-27T20:15:00+02:00 (offset is ignored)
static void handleTime() {
  int y, mo, d, h, mi, s = 0;
  String v = server.arg("value");
  if (sscanf(v.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) < 5) {
    sendResult(false, "Ungültiger Zeitstempel");
    return;
  }
  tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  t.tm_isdst = -1;
  timeval tv = {mktime(&t), 0};
  settimeofday(&tv, nullptr);
  displayRefresh();
  sendResult(true, "Zeit gesetzt");
}

static void handlePreview() {
  JsonDocument doc;
  tm t;
  char timeBuf[9] = "--:--:--";
  if (getLocalTime(&t, 0) && t.tm_year > 120) strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &t);
  doc["time"] = timeBuf;
  String px;
  displayPreview(px);
  doc["px"] = px;
  doc["mode"] = (int)displayMode();
  sendJson(doc);
}

// ---------- Setup ----------
void webSetup() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/set", handleSet);
  server.on("/api/action", handleAction);
  server.on("/api/ticker", handleTicker);
  server.on("/api/time", handleTime);
  server.on("/api/preview", HTTP_GET, handlePreview);

  // Firmware upload from the browser (WordClock-ota.bin / firmware.bin)
  server.on("/update", HTTP_POST,
    []() {
      bool ok = !Update.hasError();
      sendResult(ok, ok ? "Update OK, Neustart..." : "Update fehlgeschlagen", ok);
      if (ok) scheduleRestart(1000);
      else displaySetMode(MODE_CLOCK);
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) {
        displayStatusWord(3, "UPDATE", 0xFF8000);
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if (up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        Update.end(true);
      } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
      }
    });

  server.onNotFound([]() { sendResult(false, "Nicht gefunden"); });
  server.begin();
}

void webLoop() {
  server.handleClient();
  if (restartPending && (int32_t)(millis() - restartAt) >= 0) ESP.restart();
}
