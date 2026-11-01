// outdoor_leds.h
// Drives the two outdoor WS2812B strips (18m/540-LED "strip A" and
// 12m/360-LED "strip B") from the QT Py RP2040's own PIO-capable GPIO
// pins, feeding the QuinLED Diff-Adv Sender-4's two data input channels.
// Two independent render sources share these same two strips: the Test
// Pattern diagnostic sequence, and the active monthly preset (normal
// operation) — see qtpy_led_driver.ino for how the two take turns.
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md.
//
// Exposes:
//   initOutdoorLeds()                         — call once in setup()
//   clearOutdoorLeds()                        — turn both strips off, and
//                                                reset the render-state memo
//                                                (see below) so the next
//                                                showTestPatternOnOutdoorLeds()
//                                                call always re-renders
//                                                rather than trusting
//                                                possibly-stale memory from
//                                                a previous run
//   showTestPatternOnOutdoorLeds(elapsedMs)   — render the Test Pattern
//                                                sequence (led_test_pattern.h)
//                                                at the given elapsed time
//   showActiveSchemeOnOutdoorLeds(topColor, bottomColor, brightnessPercent)
//                                                — render the given top-
//                                                strip/bottom-strip colors
//                                                (0x00RRGGBB, as sent by the
//                                                MacroPad — see
//                                                qtpy_led_driver.ino) at the
//                                                given 0-100 brightness.
//                                                No scheme knowledge lives
//                                                here at all: the MacroPad
//                                                is the sole authority on
//                                                what any scheme looks
//                                                like (led_presets.h) and
//                                                sends colors directly, not
//                                                a scheme index to look up.

#pragma once
#include <Adafruit_NeoPixel.h>
#include "led_test_pattern.h"
#include "monthly_presets.h"

// Pin assignment is confirmed (not just a placeholder): QT Py "A0"/"A1"
// pads (GPIO29/GPIO28 per arduino-pico's adafruit_qtpy variant header),
// A0 -> the 18m strip and A1 -> the 12m strip, deliberately matching the
// installation's 1st-floor/2nd-floor order (18m = 1st floor = lower pad
// number). Still unverified at the signal level against the real wiring
// to the QuinLED Diff-Adv Sender-4 until that's actually connected.
#define STRIP_A_PIN       29  // 18m strip — 1st floor
#define STRIP_A_LED_COUNT 540
#define STRIP_B_PIN       28  // 12m strip — 2nd floor
#define STRIP_B_LED_COUNT 360

static Adafruit_NeoPixel stripA(STRIP_A_LED_COUNT, STRIP_A_PIN, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel stripB(STRIP_B_LED_COUNT, STRIP_B_PIN, NEO_GRB + NEO_KHZ800);
static TestPatternRenderState _outdoorRenderState;

static inline void initOutdoorLeds() {
  stripA.begin();
  stripA.show();
  stripB.begin();
  stripB.show();
  testPatternResetRenderState(&_outdoorRenderState);
}

static inline void clearOutdoorLeds() {
  stripA.clear();
  stripA.show();
  stripB.clear();
  stripB.show();
  testPatternResetRenderState(&_outdoorRenderState);
}

static inline void _renderOutdoorStrip(Adafruit_NeoPixel& strip, uint16_t count, uint32_t color, bool evenParityLit) {
  for (uint16_t i = 0; i < count; i++) {
    strip.setPixelColor(i, testPatternLedIsLit(i, evenParityLit) ? color : 0);
  }
  strip.show();
}

static inline void showTestPatternOnOutdoorLeds(unsigned long elapsedMs) {
  TestPatternPhase phase = testPatternPhaseAt(elapsedMs);
  if (!testPatternPhaseChanged(&_outdoorRenderState, phase)) {
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
}

static inline void _renderMonthlyStrip(Adafruit_NeoPixel& strip, uint16_t count, uint32_t color) {
  for (uint16_t i = 0; i < count; i++) {
    strip.setPixelColor(i, monthlyPresetLedIsLit(i) ? color : 0);
  }
  strip.show();
}

static inline void showActiveSchemeOnOutdoorLeds(uint32_t topColor, uint32_t bottomColor, int brightnessPercent) {
  // Top = A1/stripB/12m/2nd floor, bottom = A0/stripA/18m/1st floor — see
  // the "Pin assignment" comment above. "Off" colors (0,0,0) render as
  // dark through this same math with no special-casing needed.
  _renderMonthlyStrip(stripB, STRIP_B_LED_COUNT, monthlyPresetScaleColor(topColor, brightnessPercent));
  _renderMonthlyStrip(stripA, STRIP_A_LED_COUNT, monthlyPresetScaleColor(bottomColor, brightnessPercent));
}
