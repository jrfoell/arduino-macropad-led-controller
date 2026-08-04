// keypad_leds.h
// Drives the MacroPad's own 12 onboard NeoPixel LEDs under the keys
// (PIN_NEOPIXEL, NUM_NEOPIXEL from the board variant) — separate hardware
// from the outdoor installation's display LEDs (see led_output.h). Per the
// "Preview vs. commit" principle in
// docs/superpowers/specs/2026-08-02-ui-navigation-design.md, these are used
// to preview a pending selection/edit live; the outdoor LEDs only change
// once a selection is confirmed.
//
// Exposes:
//   initKeypadLeds()             — call once in setup()
//   showKeypadLedColors(colors)  — push 12 colors (indices 0-11, matching
//                                  keypad_input.h's raw key index order) to
//                                  the LEDs immediately
//   clearKeypadLeds()            — turn all 12 off

#pragma once

#include <Adafruit_NeoPixel.h>
#include "debug.h"

static Adafruit_NeoPixel keypadLeds(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

static inline void initKeypadLeds() {
  keypadLeds.begin();
  keypadLeds.show();
  DEBUG_PRINTLN("Keypad LEDs: initialized");
}

static inline void showKeypadLedColors(const uint32_t colors[NUM_NEOPIXEL]) {
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    keypadLeds.setPixelColor(i, colors[i]);
  }
  keypadLeds.show();
}

static inline void clearKeypadLeds() {
  keypadLeds.clear();
  keypadLeds.show();
}
