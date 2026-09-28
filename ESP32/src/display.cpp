#include "display.h"
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <vector>
#include "settings.h"
#include "layout.h"
#include "font5x7.h"

#define LED_PIN 32
#define GRID 16
#define NUM_LEDS (GRID * GRID)
#define FRAME_MS 20
#define FADE_MS 450
#define TICKER_STEP_MS 70

static Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

static uint32_t target[NUM_LEDS];   // grid order, already scaled
static uint32_t shown[NUM_LEDS];
static uint32_t fadeFrom[NUM_LEDS];
static uint32_t fadeStart = 0;
static bool fading = false;
static bool dirty = true;

static DisplayMode mode = MODE_CLOCK;
static uint32_t modeStart = 0;
static uint32_t modeUntil = 0;      // 0 = no timeout
static uint32_t lastFrame = 0;

// Status word
static uint8_t statusRow = 0;
static String statusText;
static uint32_t statusColor = 0;
static bool statusBlink = false;

// Ticker
static std::vector<uint8_t> tickerCols;
static int tickerOffset = 0;
static uint32_t tickerColorRaw = 0;
static uint32_t lastTickerStep = 0;

// Clock state
static bool timeValid = false;
static bool night = false;
static int lastYday = -1;
static int lastChimeHour = -1;
static uint32_t randomColor = 0;

// ---------- Helpers ----------
static uint16_t xyToIndex(uint8_t row, uint8_t col) {
  uint8_t r = (cfg.origin >= 2) ? GRID - 1 - row : row;
  uint8_t c = (cfg.origin & 1) ? GRID - 1 - col : col;
  if (cfg.vertical) { uint8_t t = r; r = c; c = t; }
  if (cfg.serpentine && (r & 1)) c = GRID - 1 - c;
  return r * GRID + c;
}

// Scales a color by level (0..INTENSITY_LIMIT) and the global power cap
static uint32_t scaleColor(uint32_t color, uint16_t level, float extra = 1.0f) {
  float f = (float)min<uint16_t>(level, INTENSITY_LIMIT) / INTENSITY_LIMIT * cfg.maxPower / 100.0f * extra;
  uint8_t r = ((color >> 16) & 0xFF) * f;
  uint8_t g = ((color >> 8) & 0xFF) * f;
  uint8_t b = (color & 0xFF) * f;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static uint32_t lerpColor(uint32_t a, uint32_t b, float t) {
  uint32_t out = 0;
  for (int s = 0; s <= 16; s += 8) {
    int ca = (a >> s) & 0xFF, cb = (b >> s) & 0xFF;
    out |= (uint32_t)(ca + (cb - ca) * t) << s;
  }
  return out;
}

static void clearTarget() { memset(target, 0, sizeof(target)); }

static void setCell(uint8_t row, uint8_t col, uint32_t color) {
  if (row < GRID && col < GRID) target[row * GRID + col] = color;
}

// Lights a word found in the given row, searching from column 'from'
static bool lightWord(uint8_t row, const char* word, uint32_t color, uint8_t from = 0) {
  if (row >= GRID || !word) return false;
  const char* line = LAYOUT[row];
  const char* p = strstr(line + from, word);
  if (!p) return false;
  uint8_t col = p - line;
  size_t len = strlen(word);
  for (size_t i = 0; i < len; i++) setCell(row, col + i, color);
  return true;
}

// Draws a glyph from the 5x7 font with its top-left corner at (row, col)
static void drawChar(char ch, int row, int col, uint32_t color) {
  if (ch < 32 || ch > 126) ch = '?';
  for (int x = 0; x < 5; x++) {
    uint8_t bits = pgm_read_byte(&FONT5X7[ch - 32][x]);
    for (int y = 0; y < 8; y++)
      if ((bits >> y) & 1) {
        int r = row + y, c = col + x;
        if (r >= 0 && r < GRID && c >= 0 && c < GRID) setCell(r, c, color);
      }
  }
}

// ---------- Time to words (German) ----------
struct HourWord { uint8_t row; const char* text; };
static const HourWord HOURS[12] = {
  {14, "ZWoLF"}, {13, "EINS"}, {13, "ZWEI"}, {13, "DREI"}, {13, "VIER"}, {14, "FuNF"},
  {12, "SECHS"}, {11, "SIEBEN"}, {14, "ACHT"}, {12, "NEUN"}, {12, "ZEHN"}, {12, "ELF"},
};

static void buildTime(const tm& t, uint32_t c) {
  int h = t.tm_hour % 12;
  int block = t.tm_min / 5;
  int extra = t.tm_min % 5;

  if (cfg.showItIs) {
    lightWord(3, "ES", c, 14);
    lightWord(4, "IST", c);
  }

  // Minute words live in rows 5/7/8/9, hour words in rows 11..14
  switch (block) {
    case 0:  lightWord(14, "UHR", c); break;
    case 1:  lightWord(8, "FuNF", c); lightWord(11, "NACH", c); break;
    case 2:  lightWord(7, "ZEHN", c); lightWord(11, "NACH", c); break;
    case 3:  lightWord(9, "VIERTEL", c); lightWord(11, "NACH", c); break;
    case 4:  lightWord(5, "ZWANZIG", c); lightWord(11, "NACH", c); break;
    case 5:  lightWord(8, "FuNF", c); lightWord(10, "VOR", c); lightWord(11, "HALB", c); break;
    case 6:  lightWord(11, "HALB", c); break;
    case 7:  lightWord(8, "FuNF", c); lightWord(11, "NACH", c); lightWord(11, "HALB", c); break;
    case 8:  lightWord(5, "ZWANZIG", c); lightWord(10, "VOR", c); break;
    case 9:  lightWord(9, "VIERTEL", c); lightWord(10, "VOR", c); break;
    case 10: lightWord(7, "ZEHN", c); lightWord(10, "VOR", c); break;
    case 11: lightWord(8, "FuNF", c); lightWord(10, "VOR", c); break;
  }

  // From :25 on the clock refers to the next hour
  int hourRef = (block >= 5) ? (h + 1) % 12 : h;
  if (hourRef == 1 && block == 0) lightWord(13, "EIN", c);  // "EIN UHR"
  else lightWord(HOURS[hourRef].row, HOURS[hourRef].text, c);

  // Single minutes: "+ 1..4 MINUTE(N)"
  if (cfg.singleMinutes && extra > 0) {
    char digit[2] = {char('0' + extra), 0};
    lightWord(15, "+", c);
    lightWord(15, digit, c);
    lightWord(15, extra == 1 ? "MINUTE" : "MINUTEN", c);
  }
}

static bool isDayTime(const tm& t) {
  if (cfg.dayStart == cfg.dayStop) return true;
  uint16_t m = t.tm_hour * 60 + t.tm_min;
  return (cfg.dayStart < cfg.dayStop) ? (m >= cfg.dayStart && m < cfg.dayStop)
                                      : (m >= cfg.dayStart || m < cfg.dayStop);
}

static bool readTime(tm& t) {
  return getLocalTime(&t, 0) && t.tm_year > 120;
}

// ---------- Frame builders ----------
static void frameClock() {
  tm t;
  bool ok = readTime(t);
  timeValid = ok;
  night = ok && cfg.nightMode && !isDayTime(t);

  if (ok && cfg.randomDayColors && t.tm_yday != lastYday) {
    lastYday = t.tm_yday;
    randomColor = strip.ColorHSV(esp_random() & 0xFFFF, 255, 255);
  }

  uint32_t tc = night ? cfg.timeColorNight : (cfg.randomDayColors && randomColor ? randomColor : cfg.timeColor);
  uint8_t tb = night ? cfg.timeBrightnessNight : cfg.timeBrightnessDay;
  uint32_t bc = night ? cfg.backColorNight : cfg.backColor;
  uint8_t bb = night ? cfg.backBrightnessNight : cfg.backBrightnessDay;

  uint32_t back = scaleColor(bc, bb);
  for (int i = 0; i < NUM_LEDS; i++) target[i] = back;

  // Extra words carry their brightness in the color; dim them at night like the time
  float wordFactor = 1.0f;
  if (night && cfg.timeBrightnessDay > 0)
    wordFactor = min(1.0f, (float)cfg.timeBrightnessNight / cfg.timeBrightnessDay);
  for (uint8_t i = 0; i < EXTRA_WORD_COUNT; i++) {
    if (!cfg.words[i].active) continue;
    uint32_t c = scaleColor(cfg.words[i].color, INTENSITY_LIMIT, wordFactor);
    for (const WordPart& p : EXTRA_WORDS[i].parts)
      if (p.row >= 0) lightWord(p.row, p.text, c);
  }

  if (ok) {
    buildTime(t, scaleColor(tc, tb));
    // Hourly chime: show digital time for 10 s on the full hour
    if (cfg.digitalHourChime && t.tm_min == 0 && t.tm_hour != lastChimeHour) {
      lastChimeHour = t.tm_hour;
      displayDigital(10000);
    }
    if (t.tm_min != 0) lastChimeHour = -1;
  } else {
    lightWord(5, "ZEIT", scaleColor(0x0060FF, 20));  // waiting for NTP
  }
}

static void frameDigital() {
  tm t;
  clearTarget();
  if (!readTime(t)) { lightWord(5, "ZEIT", scaleColor(0x0060FF, 20)); return; }
  uint32_t c = scaleColor(night ? cfg.timeColorNight : cfg.timeColor,
                          max<uint8_t>(night ? cfg.timeBrightnessNight : cfg.timeBrightnessDay, 5));
  char buf[3];
  snprintf(buf, sizeof(buf), "%02d", t.tm_hour);
  drawChar(buf[0], 1, 2, c);
  drawChar(buf[1], 1, 8, c);
  snprintf(buf, sizeof(buf), "%02d", t.tm_min);
  drawChar(buf[0], 9, 2, c);
  drawChar(buf[1], 9, 8, c);
}

static void frameTicker() {
  if (millis() - lastTickerStep >= TICKER_STEP_MS) {
    lastTickerStep = millis();
    tickerOffset++;
  }
  if (tickerOffset + GRID >= (int)tickerCols.size()) {
    displaySetMode(MODE_CLOCK);
    frameClock();
    return;
  }
  clearTarget();
  uint32_t c = scaleColor(tickerColorRaw, max<uint8_t>(cfg.tickerBrightness, 5));
  for (int x = 0; x < GRID; x++) {
    uint8_t bits = tickerCols[tickerOffset + x];
    for (int y = 0; y < 8; y++)
      if ((bits >> y) & 1) setCell(4 + y, x, c);
  }
}

static void frameTest() {
  uint32_t elapsed = millis() - modeStart;
  uint8_t lvl = max<uint8_t>(cfg.timeBrightnessDay, 20);
  clearTarget();
  switch (mode) {
    case MODE_LEDTEST: {
      static const uint32_t cols[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
      uint8_t step = elapsed / 800;
      if (step >= 4) { displaySetMode(MODE_CLOCK); frameClock(); return; }
      uint32_t c = scaleColor(cols[step], lvl);
      for (int i = 0; i < NUM_LEDS; i++) target[i] = c;
      break;
    }
    case MODE_CALIBRATE:
      // Top-left letter red, rest of top row blue, left column green
      for (int c = 1; c < GRID; c++) setCell(0, c, scaleColor(0x0000FF, lvl));
      for (int r = 1; r < GRID; r++) setCell(r, 0, scaleColor(0x00FF00, lvl));
      setCell(0, 0, scaleColor(0xFF0000, lvl));
      break;
    case MODE_WORDCYCLE: {
      uint8_t i = (elapsed / 1200) % EXTRA_WORD_COUNT;
      for (const WordPart& p : EXTRA_WORDS[i].parts)
        if (p.row >= 0) lightWord(p.row, p.text, scaleColor(cfg.words[i].color, INTENSITY_LIMIT));
      break;
    }
    case MODE_ALLON: {
      uint32_t c = scaleColor(cfg.timeColor, lvl);
      for (int i = 0; i < NUM_LEDS; i++) target[i] = c;
      break;
    }
    default:
      break;
  }
}

static void frameStatus() {
  clearTarget();
  if (statusBlink && (millis() / 500) % 2) return;
  lightWord(statusRow, statusText.c_str(), scaleColor(statusColor, max<uint8_t>(cfg.timeBrightnessDay, 20)));
}

// ---------- Output ----------
static void pushShown() {
  for (uint8_t r = 0; r < GRID; r++)
    for (uint8_t c = 0; c < GRID; c++)
      strip.setPixelColor(xyToIndex(r, c), shown[r * GRID + c]);
  strip.show();
}

// Chase test writes raw strip indexes and bypasses the grid mapping
static void outputChase() {
  uint16_t i = ((millis() - modeStart) / 60) % NUM_LEDS;
  strip.clear();
  strip.setPixelColor(i, scaleColor(0xFF0000, max<uint8_t>(cfg.timeBrightnessDay, 20)));
  strip.show();
}

static void output(bool allowFade) {
  static uint32_t lastTarget[NUM_LEDS];
  if (memcmp(target, lastTarget, sizeof(target)) != 0) {
    memcpy(lastTarget, target, sizeof(target));
    if (allowFade && cfg.smoothTransition) {
      memcpy(fadeFrom, shown, sizeof(shown));
      fadeStart = millis();
      fading = true;
    } else {
      memcpy(shown, target, sizeof(target));
      fading = false;
      dirty = true;
    }
  }
  if (fading) {
    float t = (float)(millis() - fadeStart) / FADE_MS;
    if (t >= 1.0f) { t = 1.0f; fading = false; }
    for (int i = 0; i < NUM_LEDS; i++) shown[i] = lerpColor(fadeFrom[i], target[i], t);
    dirty = true;
  }
  if (dirty) {
    pushShown();
    dirty = false;
  }
}

// ---------- Public API ----------
void displayInit() {
  strip.begin();
  strip.setBrightness(255);  // scaling is done per color
  strip.clear();
  strip.show();
  memset(shown, 0, sizeof(shown));
}

void displayLoop() {
  if (millis() - lastFrame < FRAME_MS) return;
  lastFrame = millis();

  if (modeUntil && (int32_t)(millis() - modeUntil) >= 0) displaySetMode(MODE_CLOCK);

  switch (mode) {
    case MODE_CLOCK:   frameClock(); output(true); break;
    case MODE_STATUS:  frameStatus(); output(false); break;
    case MODE_TICKER:  frameTicker(); output(false); break;
    case MODE_DIGITAL: frameDigital(); output(true); break;
    case MODE_CHASE:   outputChase(); dirty = true; break;
    default:           frameTest(); output(false); break;
  }
}

void displaySetMode(DisplayMode m) {
  mode = m;
  modeStart = millis();
  modeUntil = 0;
  dirty = true;
}

DisplayMode displayMode() { return mode; }

void displayRefresh() { dirty = true; }

void displayStatusWord(uint8_t row, const char* word, uint32_t color, bool blink) {
  displaySetMode(MODE_STATUS);
  statusRow = row;
  statusText = word;
  statusColor = color;
  statusBlink = blink;
  frameStatus();
  output(false);
}

void displayDigital(uint32_t durationMs) {
  displaySetMode(MODE_DIGITAL);
  modeUntil = millis() + durationMs;
}

// Converts UTF-8 umlauts to ASCII (Ä -> AE, ß -> SS, ...)
static String toAscii(const String& in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    uint8_t ch = in[i];
    if (ch < 0x80) { out += (char)ch; continue; }
    if (ch == 0xC3 && i + 1 < in.length()) {
      uint8_t n = in[++i];
      switch (n) {
        case 0x84: out += "AE"; break;
        case 0x96: out += "OE"; break;
        case 0x9C: out += "UE"; break;
        case 0xA4: out += "ae"; break;
        case 0xB6: out += "oe"; break;
        case 0xBC: out += "ue"; break;
        case 0x9F: out += "ss"; break;
        default:   out += '?'; break;
      }
    } else if (ch == 0xE2 && i + 2 < in.length() && (uint8_t)in[i + 1] == 0x82 && (uint8_t)in[i + 2] == 0xAC) {
      out += "EUR";
      i += 2;
    } else if ((ch & 0xC0) == 0xC0) {
      out += '?';  // other multi-byte sequence: skip continuation bytes
      while (i + 1 < in.length() && ((uint8_t)in[i + 1] & 0xC0) == 0x80) i++;
    }
  }
  return out;
}

void displayTicker(const String& text, int32_t color) {
  String s = toAscii(text);
  tickerCols.assign(GRID, 0);  // start off-screen right
  for (size_t i = 0; i < s.length(); i++) {
    char ch = s[i];
    if (ch < 32 || ch > 126) ch = '?';
    for (int x = 0; x < 5; x++) tickerCols.push_back(pgm_read_byte(&FONT5X7[ch - 32][x]));
    tickerCols.push_back(0);
  }
  tickerCols.insert(tickerCols.end(), GRID, 0);  // scroll fully out
  tickerOffset = 0;
  lastTickerStep = millis();
  tickerColorRaw = color >= 0 ? (uint32_t)color : cfg.tickerColor;
  displaySetMode(MODE_TICKER);
}

void displayStartupAnimation() {
  uint32_t start = millis();
  while (millis() - start < 1600) {
    uint16_t base = (millis() - start) * 60;
    for (uint8_t r = 0; r < GRID; r++)
      for (uint8_t c = 0; c < GRID; c++)
        shown[r * GRID + c] = scaleColor(strip.ColorHSV(base + (r + c) * 2048, 255, 255), 25);
    pushShown();
    delay(FRAME_MS);
  }
  clearTarget();
  lightWord(9, "WORDCLOCK", scaleColor(cfg.timeColor, max<uint8_t>(cfg.timeBrightnessDay, 20)));
  memcpy(shown, target, sizeof(shown));
  pushShown();
  delay(1200);
}

bool displayTimeValid() { return timeValid; }
bool displayIsNight() { return night; }

uint8_t displayIntensity() {
  return night ? cfg.timeBrightnessNight : cfg.timeBrightnessDay;
}

String displayNightStatus() {
  if (!cfg.nightMode) return "Night mode not used";
  return night ? "Night" : "Day";
}

void displayPreview(String& out) {
  out.reserve(NUM_LEDS * 6 + 2);
  char buf[7];
  for (int i = 0; i < NUM_LEDS; i++) {
    snprintf(buf, sizeof(buf), "%06X", (unsigned)(shown[i] & 0xFFFFFF));
    out += buf;
  }
}
