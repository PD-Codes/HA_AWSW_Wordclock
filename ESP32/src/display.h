#pragma once
#include <Arduino.h>

enum DisplayMode : uint8_t {
  MODE_CLOCK,
  MODE_STATUS,      // single status word (boot, WiFi setup, OTA)
  MODE_TICKER,      // scrolling text
  MODE_DIGITAL,     // digital HH/MM for a limited time
  MODE_LEDTEST,     // red/green/blue/white fill, then back to clock
  MODE_CALIBRATE,   // wiring calibration pattern
  MODE_CHASE,       // raw strip order
  MODE_WORDCYCLE,   // cycle all extra words
  MODE_ALLON,
};

void displayInit();
void displayLoop();

void displayStatusWord(uint8_t row, const char* word, uint32_t color = 0x0060FF, bool blink = false);
void displayTicker(const String& text, int32_t color = -1);   // -1 = configured ticker color
void displayDigital(uint32_t durationMs);
void displaySetMode(DisplayMode mode);
DisplayMode displayMode();
void displayStartupAnimation();   // blocking, ~2 s
void displayRefresh();            // redraw now (after settings changes)

bool displayTimeValid();
bool displayIsNight();
uint8_t displayIntensity();       // current time brightness 0..INTENSITY_LIMIT
String displayNightStatus();

// 256 colors in grid order (row-major, top-left first) as 6-char hex each
void displayPreview(String& out);
