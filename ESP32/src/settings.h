#pragma once
#include <Arduino.h>
#include "layout.h"

#define FW_VERSION "V6.0.0"
#define INTENSITY_LIMIT 50   // brightness values are 0..INTENSITY_LIMIT (same scale as AWSW V5)

struct WordState {
  bool active;
  uint32_t color;
};

struct Settings {
  // Colors (0xRRGGBB)
  uint32_t timeColor = 0x23A300;
  uint32_t backColor = 0x000000;
  uint32_t timeColorNight = 0xFF0000;
  uint32_t backColorNight = 0x000000;
  uint32_t tickerColor = 0xFF8000;

  // Brightness 0..INTENSITY_LIMIT
  uint8_t timeBrightnessDay = 35;
  uint8_t backBrightnessDay = 0;
  uint8_t timeBrightnessNight = 5;
  uint8_t backBrightnessNight = 0;
  uint8_t tickerBrightness = 40;
  uint8_t maxPower = 50;          // percent of full LED power (protects the power supply)

  // Day window in minutes after midnight; outside of it is night (only if nightMode)
  bool nightMode = false;
  uint16_t dayStart = 7 * 60;
  uint16_t dayStop = 22 * 60;

  // Options
  bool singleMinutes = true;
  bool showItIs = true;
  bool smoothTransition = true;
  bool digitalHourChime = false;
  bool randomDayColors = false;
  bool startupAnimation = false;
  bool showIp = false;

  // LED wiring: strip start 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right
  uint8_t origin = 1;
  bool serpentine = true;
  bool vertical = false;

  String timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
  String timeServer = "pool.ntp.org";
  String hostname = "wordclock";

  WordState words[EXTRA_WORD_COUNT];
};

extern Settings cfg;

void settingsInit();      // load from NVS (defaults for anything missing)
void settingsSave();
void resetExtraWords();   // default colors, all off

int extraWordIndex(uint8_t id);  // -1 if unknown

String colorToHex(uint32_t c);
bool hexToColor(const String& s, uint32_t& out);
String minutesToHHMM(uint16_t m);
bool hhmmToMinutes(const String& s, uint16_t& out);
