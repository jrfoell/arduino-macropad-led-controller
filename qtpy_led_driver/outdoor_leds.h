// outdoor_leds.h
// Drives the two outdoor WS2812B strips (18m/540-LED "strip A" and
// 12m/360-LED "strip B") from the QT Py RP2040's own PIO-capable GPIO
// pins, feeding the QuinLED Diff-Adv Sender-4's two data input channels.
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md.
//
// Exposes:
//   initOutdoorLeds()                         — call once in setup()
//   clearOutdoorLeds()                        — turn both strips off
//   showTestPatternOnOutdoorLeds(elapsedMs)   — render the Test Pattern
//                                                sequence (led_test_pattern.h)
//                                                at the given elapsed time

#pragma once
#include <Adafruit_NeoPixel.h>
#include "led_test_pattern.h"

// Pin numbers are QT Py "A0"/"A1" pads (GPIO29/GPIO28 per arduino-pico's
// adafruit_qtpy variant header) — NOT YET VERIFIED against real wiring,
// per this project's existing convention; confirm once hardware exists.
#define STRIP_A_PIN       29  // 18m strip
#define STRIP_A_LED_COUNT 540
#define STRIP_B_PIN       28  // 12m strip
#define STRIP_B_LED_COUNT 360

static Adafruit_NeoPixel stripA(STRIP_A_LED_COUNT, STRIP_A_PIN, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel stripB(STRIP_B_LED_COUNT, STRIP_B_PIN, NEO_GRB + NEO_KHZ800);

static inline void initOutdoorLeds() {
  stripA.begin();
  stripA.show();
  stripB.begin();
  stripB.show();
}

static inline void clearOutdoorLeds() {
  stripA.clear();
  stripA.show();
  stripB.clear();
  stripB.show();
}

static inline void _renderOutdoorStrip(Adafruit_NeoPixel& strip, uint16_t count, uint32_t color, bool evenParityLit) {
  for (uint16_t i = 0; i < count; i++) {
    strip.setPixelColor(i, testPatternLedIsLit(i, evenParityLit) ? color : 0);
  }
  strip.show();
}

static inline void showTestPatternOnOutdoorLeds(unsigned long elapsedMs) {
  static TestPatternPhase lastPhase;
  static bool havePhase = false;

  TestPatternPhase phase = testPatternPhaseAt(elapsedMs);
  if (havePhase && phase.color == lastPhase.color && phase.evenParityLit == lastPhase.evenParityLit) {
    return;
  }

  uint8_t level = (uint16_t)255 * LED_TEST_PATTERN_BRIGHTNESS_PERCENT / 100;
  uint32_t color;
  switch (phase.color) {
    case TEST_PATTERN_COLOR_RED:   color = Adafruit_NeoPixel::Color(level, 0, 0); break;
    case TEST_PATTERN_COLOR_GREEN: color = Adafruit_NeoPixel::Color(0, level, 0); break;
    default:                       color = Adafruit_NeoPixel::Color(0, 0, level); break;
  }

  _renderOutdoorStrip(stripA, STRIP_A_LED_COUNT, color, phase.evenParityLit);
  _renderOutdoorStrip(stripB, STRIP_B_LED_COUNT, color, phase.evenParityLit);

  lastPhase = phase;
  havePhase = true;
}
