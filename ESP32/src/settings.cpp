#include "settings.h"
#include <Preferences.h>

Settings cfg;
static Preferences prefs;

// Bump when the Settings layout changes in an incompatible way
static const uint32_t SETTINGS_MAGIC = 0x57430601;

void resetExtraWords() {
  for (uint8_t i = 0; i < EXTRA_WORD_COUNT; i++) {
    cfg.words[i].active = false;
    cfg.words[i].color = EXTRA_WORDS[i].defaultColor;
  }
}

void settingsInit() {
  resetExtraWords();
  prefs.begin("wordclock", true);
  if (prefs.getUInt("magic", 0) != SETTINGS_MAGIC) {
    prefs.end();
    settingsSave();  // first boot: persist defaults
    return;
  }
  cfg.timeColor = prefs.getUInt("tc", cfg.timeColor);
  cfg.backColor = prefs.getUInt("bc", cfg.backColor);
  cfg.timeColorNight = prefs.getUInt("tcn", cfg.timeColorNight);
  cfg.backColorNight = prefs.getUInt("bcn", cfg.backColorNight);
  cfg.tickerColor = prefs.getUInt("tkc", cfg.tickerColor);
  cfg.timeBrightnessDay = prefs.getUChar("tbd", cfg.timeBrightnessDay);
  cfg.backBrightnessDay = prefs.getUChar("bbd", cfg.backBrightnessDay);
  cfg.timeBrightnessNight = prefs.getUChar("tbn", cfg.timeBrightnessNight);
  cfg.backBrightnessNight = prefs.getUChar("bbn", cfg.backBrightnessNight);
  cfg.tickerBrightness = prefs.getUChar("tkb", cfg.tickerBrightness);
  cfg.maxPower = prefs.getUChar("maxp", cfg.maxPower);
  cfg.nightMode = prefs.getBool("night", cfg.nightMode);
  cfg.dayStart = prefs.getUShort("dstart", cfg.dayStart);
  cfg.dayStop = prefs.getUShort("dstop", cfg.dayStop);
  cfg.singleMinutes = prefs.getBool("single", cfg.singleMinutes);
  cfg.showItIs = prefs.getBool("itis", cfg.showItIs);
  cfg.smoothTransition = prefs.getBool("smooth", cfg.smoothTransition);
  cfg.digitalHourChime = prefs.getBool("chime", cfg.digitalHourChime);
  cfg.randomDayColors = prefs.getBool("random", cfg.randomDayColors);
  cfg.startupAnimation = prefs.getBool("startup", cfg.startupAnimation);
  cfg.showIp = prefs.getBool("showip", cfg.showIp);
  cfg.origin = prefs.getUChar("origin", cfg.origin);
  cfg.serpentine = prefs.getBool("serp", cfg.serpentine);
  cfg.vertical = prefs.getBool("vert", cfg.vertical);
  cfg.timeZone = prefs.getString("tz", cfg.timeZone);
  cfg.timeServer = prefs.getString("ntp", cfg.timeServer);
  cfg.hostname = prefs.getString("host", cfg.hostname);

  // Words are stored as a blob; a shorter blob (older firmware with fewer words) is fine.
  WordState stored[EXTRA_WORD_COUNT];
  size_t len = prefs.getBytesLength("words");
  if (len > 0 && len <= sizeof(stored) && len % sizeof(WordState) == 0) {
    prefs.getBytes("words", stored, len);
    memcpy(cfg.words, stored, len);
  }
  prefs.end();
}

void settingsSave() {
  prefs.begin("wordclock", false);
  prefs.putUInt("magic", SETTINGS_MAGIC);
  prefs.putUInt("tc", cfg.timeColor);
  prefs.putUInt("bc", cfg.backColor);
  prefs.putUInt("tcn", cfg.timeColorNight);
  prefs.putUInt("bcn", cfg.backColorNight);
  prefs.putUInt("tkc", cfg.tickerColor);
  prefs.putUChar("tbd", cfg.timeBrightnessDay);
  prefs.putUChar("bbd", cfg.backBrightnessDay);
  prefs.putUChar("tbn", cfg.timeBrightnessNight);
  prefs.putUChar("bbn", cfg.backBrightnessNight);
  prefs.putUChar("tkb", cfg.tickerBrightness);
  prefs.putUChar("maxp", cfg.maxPower);
  prefs.putBool("night", cfg.nightMode);
  prefs.putUShort("dstart", cfg.dayStart);
  prefs.putUShort("dstop", cfg.dayStop);
  prefs.putBool("single", cfg.singleMinutes);
  prefs.putBool("itis", cfg.showItIs);
  prefs.putBool("smooth", cfg.smoothTransition);
  prefs.putBool("chime", cfg.digitalHourChime);
  prefs.putBool("random", cfg.randomDayColors);
  prefs.putBool("startup", cfg.startupAnimation);
  prefs.putBool("showip", cfg.showIp);
  prefs.putUChar("origin", cfg.origin);
  prefs.putBool("serp", cfg.serpentine);
  prefs.putBool("vert", cfg.vertical);
  prefs.putString("tz", cfg.timeZone);
  prefs.putString("ntp", cfg.timeServer);
  prefs.putString("host", cfg.hostname);
  prefs.putBytes("words", cfg.words, sizeof(cfg.words));
  prefs.end();
}

int extraWordIndex(uint8_t id) {
  for (uint8_t i = 0; i < EXTRA_WORD_COUNT; i++)
    if (EXTRA_WORDS[i].id == id) return i;
  return -1;
}

String colorToHex(uint32_t c) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%06X", (unsigned)(c & 0xFFFFFF));
  return String(buf);
}

bool hexToColor(const String& s, uint32_t& out) {
  String h = s;
  h.trim();
  if (h.startsWith("#")) h = h.substring(1);
  if (h.length() != 6) return false;
  for (char ch : h)
    if (!isxdigit((unsigned char)ch)) return false;
  out = strtoul(h.c_str(), nullptr, 16) & 0xFFFFFF;
  return true;
}

String minutesToHHMM(uint16_t m) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", (m / 60) % 24, m % 60);
  return String(buf);
}

bool hhmmToMinutes(const String& s, uint16_t& out) {
  int h, m;
  if (sscanf(s.c_str(), "%d:%d", &h, &m) != 2 || h < 0 || h > 23 || m < 0 || m > 59) return false;
  out = h * 60 + m;
  return true;
}
