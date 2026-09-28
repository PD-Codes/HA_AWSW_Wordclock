// WordClock 16x16 - custom firmware for the AWSW WordClock hardware
// ESP32 + 256x WS2812B (data on GPIO32, strip starts top-right, zig-zag).
//
// Features: WiFi setup via WPS (3 min) with hotspot fallback, NTP time with DST, mDNS, web UI with live preview,
// 27 controllable extra words, day/night colors, ticker, digital time, wiring calibration,
// browser + ArduinoOTA updates. HTTP API is compatible with the AWSW V5 API
// (Home Assistant integration HA_AWSW_Wordclock).

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "settings.h"
#include "display.h"
#include "web.h"
#include "wifi_setup.h"

// Runs once when the WiFi connection is first established
static void onOnline() {
  Serial.printf("Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  applyTimeConfig();
  if (MDNS.begin(cfg.hostname.c_str())) MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname(cfg.hostname.c_str());
  ArduinoOTA.onStart([]() { displayStatusWord(3, "UPDATE", 0xFF8000); });
  ArduinoOTA.begin();

  webSetup();

  displayStatusWord(15, "OK", 0x00FF00);
  delay(800);
  displaySetMode(MODE_CLOCK);
  if (cfg.showIp) displayTicker("IP " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  settingsInit();
  displayInit();
  if (cfg.startupAnimation) displayStartupAnimation();
  wifiSetupBegin();
}

void loop() {
  if (wifiSetupLoop()) onOnline();
  if (wifiState() == WIFI_ONLINE) {
    ArduinoOTA.handle();
    webLoop();
  }
  displayLoop();
}
