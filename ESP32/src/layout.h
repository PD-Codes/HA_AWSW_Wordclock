#pragma once
#include <Arduino.h>

// Front plate letter layout (16x16), top row first.
// Umlauts use lowercase placeholders so each cell is exactly one char:
//   'u' = Ü, 'o' = Ö
// Word positions are looked up at runtime, so fixing a letter here is enough.
static const char* const LAYOUT[16] = {
  "ALARMGEBURTSTAGW",  // 0
  "MuLLAUTOFEIERTAG",  // 1
  "AFORMEL1DOWNLOAD",  // 2
  "WLANUPDATERAUSES",  // 3
  "BRINGENISTGELBER",  // 4
  "SACKZEITZWANZIGF",  // 5
  "HALBGURLAUBGENAU",  // 6
  "ZEHNWERKSTATTZUM",  // 7
  "FuNFRISEURZOCKEN",  // 8
  "WORDCLOCKVIERTEL",  // 9
  "VORNEUSTARTERMIN",  // 10
  "NACHLHALBVSIEBEN",  // 11
  "SECHSNEUNZEHNELF",  // 12
  "EINSDREIVIERZWEI",  // 13
  "ACHTZWoLFuNFUUHR",  // 14  (ZWÖLF and FÜNF share the F)
  "S+1234OKMINUTENW",  // 15
};

struct WordPart {
  int8_t row;          // -1 = unused
  const char* text;
};

// Extra words: ids 1..12 match the original AWSW firmware (keeps Home Assistant entities),
// ids 13+ are new. Names follow the original "<id>: <WORD>" format.
struct ExtraWord {
  uint8_t id;
  const char* name;
  uint32_t defaultColor;
  WordPart parts[3];
};

#define NP {-1, nullptr}
static const ExtraWord EXTRA_WORDS[] = {
  {1,  "1: ALARM",               0xCC0000, {{0, "ALARM"}, NP, NP}},
  {2,  "2: GEBURTSTAG",          0x04FF00, {{0, "GEBURTSTAG"}, NP, NP}},
  {3,  "3: MUELL RAUS BRINGEN",  0x7E008F, {{1, "MuLL"}, {3, "RAUS"}, {4, "BRINGEN"}}},
  {4,  "4: AUTO",                0x0000B3, {{1, "AUTO"}, NP, NP}},
  {5,  "5: FEIERTAG",            0x699759, {{1, "FEIERTAG"}, NP, NP}},
  {6,  "6: FORMEL1",             0x0000FF, {{2, "FORMEL1"}, NP, NP}},
  {7,  "7: GELBER SACK",         0x5C5C00, {{4, "GELBER"}, {5, "SACK"}, NP}},
  {8,  "8: URLAUB",              0x0011FF, {{6, "URLAUB"}, NP, NP}},
  {9,  "9: WERKSTATT",           0x8A0000, {{7, "WERKSTATT"}, NP, NP}},
  {10, "10: ZEIT ZUM ZOCKEN",    0x7A7A7A, {{5, "ZEIT"}, {7, "ZUM"}, {8, "ZOCKEN"}}},
  {11, "11: FRISEUR",            0x7A219E, {{8, "FRISEUR"}, NP, NP}},
  {12, "12: TERMIN",             0xFF0000, {{10, "TERMIN"}, NP, NP}},
  {13, "13: OK",                 0x00FF00, {{15, "OK"}, NP, NP}},
  {14, "14: WLAN",               0x0060FF, {{3, "WLAN"}, NP, NP}},
  {15, "15: UPDATE",             0xFF8000, {{3, "UPDATE"}, NP, NP}},
  {16, "16: DOWNLOAD",           0x00A0FF, {{2, "DOWNLOAD"}, NP, NP}},
  {17, "17: NEUSTART",           0xFF4000, {{10, "NEUSTART"}, NP, NP}},
  {18, "18: WORDCLOCK",          0xFFFFFF, {{9, "WORDCLOCK"}, NP, NP}},
  {19, "19: GENAU",              0xFFFF00, {{6, "GENAU"}, NP, NP}},
  {20, "20: ZEIT",               0xFFFFFF, {{5, "ZEIT"}, NP, NP}},
  {21, "21: MUELL",              0x7E008F, {{1, "MuLL"}, NP, NP}},
  {22, "22: RAUS",               0x7E008F, {{3, "RAUS"}, NP, NP}},
  {23, "23: BRINGEN",            0x7E008F, {{4, "BRINGEN"}, NP, NP}},
  {24, "24: GELBER",             0x5C5C00, {{4, "GELBER"}, NP, NP}},
  {25, "25: SACK",               0x5C5C00, {{5, "SACK"}, NP, NP}},
  {26, "26: ZUM",                0x7A7A7A, {{7, "ZUM"}, NP, NP}},
  {27, "27: ZOCKEN",             0x7A7A7A, {{8, "ZOCKEN"}, NP, NP}},
};
#undef NP
static const uint8_t EXTRA_WORD_COUNT = sizeof(EXTRA_WORDS) / sizeof(EXTRA_WORDS[0]);
