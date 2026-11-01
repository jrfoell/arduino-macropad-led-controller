// monthly_presets.h
// Hardware-agnostic *rendering math* for the monthly outdoor LED presets,
// shared by the MacroPad (keypad-LED preview, led_presets.h) and the QT Py
// (real outdoor strip rendering, outdoor_leds.h).
//
// This file deliberately holds no per-month content (colors, which scheme
// means what) — the MacroPad is the sole authority on that (see
// led_presets.h) and sends the full visual recipe to the QT Py over I2C
// (MacropadShared/src/i2c_protocol.h's QTPY_CMD_SET_SCHEME) rather than the
// QT Py having its own compiled-in copy. That's the whole point: tuning a
// color only ever means editing+reflashing the MacroPad, never the QT Py.
// What's left here is just the math both sides still need to apply
// whatever colors they're given identically.
//
// Exposes:
//   MONTHLY_PRESET_LED_STRIDE    — every Nth outdoor LED is lit, spacing
//                                  the lights out along the strip. Applies
//                                  to every scheme — a single global knob.
//   monthlyPresetLedIsLit(i, offset) — true if outdoor LED index i should
//                                  be lit under that stride, shifted by
//                                  `offset` (0..stride-1). The QT Py
//                                  advances this once per boot
//                                  (scheme_storage.h's
//                                  loadAndAdvanceLedOffset()) so it isn't
//                                  always the same physical LEDs doing the
//                                  work — spreads wear/usage hours evenly
//                                  and helps surface a dead LED instead of
//                                  it silently always being a dark one.
//   monthlyPresetScaleColor(c, percent) — c scaled to percent (0-100) of
//                                  its original brightness; same math as
//                                  the MacroPad's own colors.h, duplicated
//                                  here since the QT Py sketch can't
//                                  include that file

#pragma once
#include <stdint.h>

#define MONTHLY_PRESET_LED_STRIDE 3  // every 3rd LED lit, spacing the lights out

static inline bool monthlyPresetLedIsLit(uint16_t ledIndex, uint8_t offset) {
  return (ledIndex % MONTHLY_PRESET_LED_STRIDE) == (offset % MONTHLY_PRESET_LED_STRIDE);
}

static inline uint32_t monthlyPresetScaleColor(uint32_t color, uint8_t percent) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8)  & 0xFF;
  uint8_t b = color & 0xFF;
  r = (uint16_t)r * percent / 100;
  g = (uint16_t)g * percent / 100;
  b = (uint16_t)b * percent / 100;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
