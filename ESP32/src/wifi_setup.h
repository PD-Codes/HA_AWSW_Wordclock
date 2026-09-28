#pragma once
#include <Arduino.h>

// WiFi setup flow (non-blocking, call wifiSetupLoop() from loop()):
//   1. Stored credentials -> connect (30 s)
//   2. No credentials / connect failed -> WPS push button for 3 min   (WLAN blinks orange)
//   3. WPS unsuccessful -> setup hotspot "WordClock-Setup" for 5 min  (WLAN blinks red)
//   4. Hotspot timeout -> restart and start over
// While connecting the word WLAN is shown in blue.

enum WifiState : uint8_t { WIFI_CONNECTING, WIFI_WPS, WIFI_PORTAL, WIFI_ONLINE };

void wifiSetupBegin();
// Returns true exactly once, when the connection is first established
bool wifiSetupLoop();
WifiState wifiState();
void wifiStartWps();   // manual start (e.g. from the web UI)
