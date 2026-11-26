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
//   monthlyPresetPaletteColorAt(i, offset, palette) — the color outdoor
//                                  LED i should show: 0 (off) if it isn't
//                                  lit under the stride/offset above,
//                                  otherwise the palette color for its
//                                  position among the lit LEDs — the 1st
//                                  lit LED gets colors[0], the 2nd
//                                  colors[1], ... wrapping around, so a
//                                  palette cycles evenly along the strip.
//                                  An empty palette (count 0) is all off.
//   monthlyPresetScaleColor(c, percent) — c scaled to percent (0-100) of
//                                  its original brightness; same math as
//                                  the MacroPad's own colors.h, duplicated
//                                  here since the QT Py sketch can't
//                                  include that file

#pragma once
#include <stdint.h>
#include "scheme_recipe.h"

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

static inline uint32_t monthlyPresetPaletteColorAt(uint16_t ledIndex, uint8_t offset, const SchemePalette* palette) {
  if (palette->count == 0 || !monthlyPresetLedIsLit(ledIndex, offset)) {
    return 0;
  }
  // Lit LEDs sit at offset, offset+stride, offset+2*stride, ... so
  // ledIndex / stride is each one's 0,1,2,... position among the lit ones.
  return palette->colors[(ledIndex / MONTHLY_PRESET_LED_STRIDE) % palette->count];
}
