#include "wifi_setup.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <esp_wps.h>
#include "settings.h"
#include "display.h"

#define CONNECT_TIMEOUT_MS  30000UL
#define WPS_TIMEOUT_MS      180000UL   // 3 min, like the original firmware
#define PORTAL_TIMEOUT_MS   300000UL   // 5 min, then restart and try again
#define PORTAL_SSID         "WordClock-Setup"

static WifiState state = WIFI_CONNECTING;
static uint32_t stateStart = 0;
static WiFiManager wm;

// Set from the WiFi event task, handled in the main loop
static volatile bool wpsSuccess = false;
static volatile bool wpsRetry = false;

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WPS_ER_SUCCESS: wpsSuccess = true; break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
    case ARDUINO_EVENT_WPS_ER_PBC_OVERLAP: wpsRetry = true; break;
    default: break;
  }
}

static bool hasStoredCredentials() {
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return false;
  return conf.sta.ssid[0] != 0;
}

static void enter(WifiState s) {
  state = s;
  stateStart = millis();
}

static void startConnect() {
  displayStatusWord(3, "WLAN", 0x0060FF);
  WiFi.begin();  // uses the stored credentials
  enter(WIFI_CONNECTING);
}

static void wpsRun() {
  esp_wps_config_t conf = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
  esp_wifi_wps_disable();
  if (esp_wifi_wps_enable(&conf) == ESP_OK) esp_wifi_wps_start(0);
}

void wifiStartWps() {
  Serial.println("WiFi: WPS active - press the WPS button on your router");
  WiFi.disconnect();
  wpsSuccess = wpsRetry = false;
  displayStatusWord(3, "WLAN", 0xFF8000, true);
  wpsRun();
  enter(WIFI_WPS);
}

static void startPortal() {
  esp_wifi_wps_disable();
  Serial.println("WiFi: WPS timed out, opening setup hotspot " PORTAL_SSID);
  displayStatusWord(3, "WLAN", 0xFF0000, true);
  wm.setConfigPortalBlocking(false);
  wm.setHostname(cfg.hostname.c_str());
  wm.setTitle("WordClock");
  wm.startConfigPortal(PORTAL_SSID);
  enter(WIFI_PORTAL);
}

void wifiSetupBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setHostname(cfg.hostname.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.onEvent(onWifiEvent);
  if (hasStoredCredentials()) startConnect();
  else wifiStartWps();
}

bool wifiSetupLoop() {
  switch (state) {
    case WIFI_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) { enter(WIFI_ONLINE); return true; }
      if (millis() - stateStart > CONNECT_TIMEOUT_MS) wifiStartWps();
      break;

    case WIFI_WPS:
      if (wpsSuccess) {
        wpsSuccess = false;
        esp_wifi_wps_disable();
        Serial.printf("WiFi: WPS OK, connecting to %s\n", WiFi.SSID().c_str());
        startConnect();
      } else if (millis() - stateStart > WPS_TIMEOUT_MS) {
        startPortal();
      } else if (wpsRetry) {
        wpsRetry = false;
        wpsRun();
      }
      break;

    case WIFI_PORTAL:
      if (wm.process()) {  // credentials saved and connected
        enter(WIFI_ONLINE);
        return true;
      }
      if (millis() - stateStart > PORTAL_TIMEOUT_MS) {
        displayStatusWord(10, "NEUSTART");
        delay(500);
        ESP.restart();
      }
      break;

    case WIFI_ONLINE:
      break;
  }
  return false;
}

WifiState wifiState() { return state; }
