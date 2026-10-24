// oled_display.h
// Drives the MacroPad's onboard 128x64 SH1106G OLED for the verb/noun UI.
// Screen is split into three fixed zones, each an integer number of 8px
// text lines (text size 1, no wrap, white-on-black — per Adafruit's own
// Arduino example for this board, see "OLED font convention" in
// docs/superpowers/specs/2026-08-02-ui-navigation-design.md):
//   - Menu zone    (rows 0-5, y=0-47)  — top-level menu or preset list
//   - Message zone (row 6,   y=48-55) — transient hints / "confirm" prompt
//   - Input zone   (row 7,   y=56-63) — persistent verb/noun entry text
// Every row is space-padded to the full 21-char line width so redrawing it
// always overwrites what was there before, with no flicker-inducing
// clearDisplay() needed.
//
// Exposes:
//   initOledDisplay()                                  — call once in setup()
//   renderMenuTop(highlightIndex)                       — 0=Brightness,
//                                                          1=Presets, 2=Custom
//   renderMenuPresetList(highlightMonth, names, count)  — 1-based month,
//                                                          scrolls a 6-row window
//   renderMenuBrightness(percent)                       — 0-100, text progress bar
//   renderMessage(msg)                                  — "" clears the zone
//   renderInputLine(text)                               — "" clears the zone

#pragma once

#include <Adafruit_SH110X.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"

#define OLED_TEXT_COLS  21   // 128px / 6px-per-glyph at text size 1
#define OLED_MENU_ROWS  6    // rows 0-5
#define OLED_MSG_ROW    6
#define OLED_INPUT_ROW  7

static Adafruit_SH1106G display(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);

static inline void initOledDisplay() {
  display.begin(0, true);
  display.clearDisplay();
  display.display();
  DEBUG_PRINTLN("OLED: initialized");
}

// Draws one 8px text row, space-padded to the full line width. Does not
// flush — callers batch multiple rows, then call display.display() once.
static inline void _drawRow(uint8_t row, const char* text) {
  char buf[OLED_TEXT_COLS + 1];
  size_t len = strlen(text);
  if (len > OLED_TEXT_COLS) len = OLED_TEXT_COLS;
  memcpy(buf, text, len);
  for (size_t i = len; i < OLED_TEXT_COLS; i++) buf[i] = ' ';
  buf[OLED_TEXT_COLS] = '\0';

  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  display.setCursor(0, row * 8);
  display.print(buf);
}

static inline void renderMenuTop(int highlightIndex) {
  static const char* const TOP_MENU_ITEMS[3] = {"Brightness", "Presets", "Custom"};
  char line[OLED_TEXT_COLS + 1];
  for (int row = 0; row < OLED_MENU_ROWS; row++) {
    if (row < 3) {
      snprintf(line, sizeof(line), "%c%s", (row == highlightIndex) ? '>' : ' ', TOP_MENU_ITEMS[row]);
      _drawRow(row, line);
    } else {
      _drawRow(row, "");
    }
  }
  display.display();
}

// percent is 0-100. Renders a label row and a text progress bar row (e.g.
// "[#########     ] 68%"), then blanks the rest of the menu zone.
static inline void renderMenuBrightness(int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  const int barWidth = 14;
  int filled = (percent * barWidth + 50) / 100;  // rounded
  char bar[barWidth + 1];
  for (int i = 0; i < barWidth; i++) bar[i] = (i < filled) ? '#' : ' ';
  bar[barWidth] = '\0';

  char line[OLED_TEXT_COLS + 1];
  _drawRow(0, "Brightness");
  snprintf(line, sizeof(line), "[%s] %3d%%", bar, percent);
  _drawRow(1, line);
  for (int row = 2; row < OLED_MENU_ROWS; row++) _drawRow(row, "");
  display.display();
}

// highlightMonth is 1-based (1=January ... 12=December), matching
// led_presets.h's scheme numbering used throughout the UI/navigation design.
static inline void renderMenuPresetList(int highlightMonth, const char* const names[], int count) {
  int selectedIdx = highlightMonth - 1;  // 0-based
  int windowStart = selectedIdx - (OLED_MENU_ROWS / 2);
  if (windowStart < 0) windowStart = 0;
  if (windowStart > count - OLED_MENU_ROWS) windowStart = count - OLED_MENU_ROWS;
  if (windowStart < 0) windowStart = 0;  // count < OLED_MENU_ROWS safety

  char line[OLED_TEXT_COLS + 1];
  for (int row = 0; row < OLED_MENU_ROWS; row++) {
    int schemeIdx = windowStart + row;
    if (schemeIdx < count) {
      snprintf(line, sizeof(line), "%c%s", (schemeIdx == selectedIdx) ? '>' : ' ', names[schemeIdx]);
      _drawRow(row, line);
    } else {
      _drawRow(row, "");
    }
  }
  display.display();
}

static inline void renderMessage(const char* msg) {
  _drawRow(OLED_MSG_ROW, msg ? msg : "");
  display.display();
}

static inline void renderInputLine(const char* text) {
  _drawRow(OLED_INPUT_ROW, text ? text : "");
  display.display();
}
