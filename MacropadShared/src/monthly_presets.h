// monthly_presets.h
// Hardware-agnostic data/logic for the monthly outdoor LED presets, shared
// by the MacroPad (keypad-LED preview, led_presets.h) and the QT Py (real
// outdoor strip rendering, outdoor_leds.h + scheme_storage.h) — living here
// rather than duplicated means the keypad preview can't silently drift out
// of sync with what the strips actually show.
//
// Only October (scheme index 9, matching led_presets.h's SCHEME_NAMES
// numbering) is implemented so far — every other month is future work, one
// month at a time, same as led_presets.h's existing preview-only palette.
//
// Exposes:
//   MONTHLY_PRESET_LED_STRIDE    — every Nth outdoor LED is lit, spacing
//                                  the lights out along the strip. Applies
//                                  to every implemented month, not just
//                                  October — a single global knob.
//   monthlyPresetLedIsLit(i)     — true if outdoor LED index i should be
//                                  lit under that stride
//   monthlyPresetScaleColor(c, percent) — c scaled to percent (0-100) of
//                                  its original brightness; same math as
//                                  the MacroPad's own colors.h, duplicated
//                                  here since the QT Py sketch can't
//                                  include that file
//   MONTHLY_PRESET_OCTOBER_TOP_COLOR    — October's "top strip" (physically
//                                  the 2nd floor, QT Py pad A1) color:
//                                  purple — matches colors.h's PIXEL_PURPLE
//   MONTHLY_PRESET_OCTOBER_BOTTOM_COLOR — October's "bottom strip"
//                                  (1st floor, QT Py pad A0) color: a
//                                  red-leaning, slightly darker orange —
//                                  tuned after seeing colors.h's plain
//                                  PIXEL_ORANGE read as too close to yellow
//                                  on real hardware; led_presets.h's
//                                  October preview uses this exact value
//                                  too (not PIXEL_ORANGE) so the two can't
//                                  drift apart again

#pragma once
#include <stdint.h>

#define MONTHLY_PRESET_LED_STRIDE 3  // every 3rd LED lit, spacing the lights out

static inline bool monthlyPresetLedIsLit(uint16_t ledIndex) {
  return (ledIndex % MONTHLY_PRESET_LED_STRIDE) == 0;
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

// 0x00RRGGBB packing, matching colors.h / Adafruit_NeoPixel::Color() — full
// intensity; monthlyPresetScaleColor() applies the confirmed brightness.
#define MONTHLY_PRESET_OCTOBER_TOP_COLOR    ((uint32_t)0x00640096)  // Color(100,   0, 150) — purple
// Color(200,80,0) [colors.h's PIXEL_ORANGE] -> Color(180,50,0) [too close to
// yellow -> good step toward red] -> Color(160,20,0) [this one: the same
// -20R/-30G/+0B delta applied again, for a deeper, more saturated red-orange].
#define MONTHLY_PRESET_OCTOBER_BOTTOM_COLOR ((uint32_t)0x00A01400)  // Color(160,  20,   0)
