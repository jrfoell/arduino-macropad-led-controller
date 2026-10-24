// colors.h
// Named color constants for the MacroPad's 12 onboard NeoPixels (one per
// key), plus a small reusable color-math helper. Values match
// Adafruit_NeoPixel::Color(r, g, b) packing (0x00RRGGBB) — the library
// handles GRB reordering on output, so use these constants everywhere
// rather than raw hex.
//
// The constants below cover only the MacroPad's own fixed indicator colors
// (e.g. "this key/preset is active"). Per-scheme color palettes for the
// outdoor installation belong in led_presets.h once each month's design is
// decided — keep this file limited to small, reusable status colors and
// color utilities.
//
// Exposes:
//   scaleColorBrightness(color, percent) — returns `color` scaled to
//                                           `percent` (0-100) of its
//                                           original brightness

#pragma once

#define PIXEL_OFF     ((uint32_t)0)
#define PIXEL_RED     ((uint32_t)0x00C80000)  // Color(200,   0,   0)
#define PIXEL_GREEN   ((uint32_t)0x0000C800)  // Color(  0, 200,   0)
#define PIXEL_BLUE    ((uint32_t)0x000000C8)  // Color(  0,   0, 200)
#define PIXEL_WHITE   ((uint32_t)0x00646464)  // Color(100, 100, 100)
#define PIXEL_ORANGE  ((uint32_t)0x00C85000)  // Color(200,  80,   0)
#define PIXEL_PURPLE  ((uint32_t)0x00640096)  // Color(100,   0, 150)
#define PIXEL_YELLOW  ((uint32_t)0x00C8C800)  // Color(200, 200,   0)

static inline uint32_t scaleColorBrightness(uint32_t color, uint8_t percent) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8)  & 0xFF;
  uint8_t b = color & 0xFF;
  r = (uint16_t)r * percent / 100;
  g = (uint16_t)g * percent / 100;
  b = (uint16_t)b * percent / 100;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
