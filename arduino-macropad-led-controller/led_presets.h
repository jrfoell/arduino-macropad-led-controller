// led_presets.h
// Defines the 12 monthly lighting schemes and runs whichever one is active.
// Only October's actual *outdoor installation* behavior is decided and
// implemented so far — the other 11 months are still an open design
// question (see CLAUDE.md's "Open design questions"). The real outdoor
// rendering doesn't happen here or via led_output.h, though: the MacroPad
// doesn't drive the outdoor strips directly at all — on confirm,
// ui_controller.h sends the scheme index + brightness to the QT Py over
// I2C (qtpy_link.h's setActiveSchemeOnQtpy()), which renders and persists
// it independently (qtpy_led_driver/outdoor_leds.h + scheme_storage.h).
// getSchemePreviewColors() is this file's own piece: a color swatch per
// month for the onboard keypad LEDs (see keypad_leds.h), used by
// ui_controller.h as a live preview while browsing presets — for October
// this now mirrors the real outdoor look (see its case below); for every
// other month it's still just a rough prototype swatch.
//
// Exposes:
//   initSchemes()                       — call once in setup()
//   setActiveScheme(index)              — index 0-11, one per month (0 = January)
//   getActiveScheme()                   — current index
//   renderActiveScheme()                — call every loop(); renders the
//                                          active scheme via led_output.h
//   getSchemePreviewColors(index, out)  — fills out[NUM_NEOPIXEL] with a
//                                          preview swatch for scheme `index`

#pragma once

#include "debug.h"
#include "led_output.h"
#include "colors.h"
#include <monthly_presets.h>  // from MacropadShared — October's real colors, kept in sync with the strips
#include <Adafruit_NeoPixel.h>

#define NUM_SCHEMES 12

// TODO: each month's theme — what "significant" event it celebrates, and
// what the scheme actually looks like — is not decided yet. Placeholder
// names only.
static const char* const SCHEME_NAMES[NUM_SCHEMES] = {
  "January",   "February", "March",    "April",
  "May",       "June",     "July",     "August",
  "September", "October",  "November", "December"
};

static int _activeScheme = 0;

static inline void initSchemes() {
  initLedOutput();
  DEBUG_PRINTLN("Schemes: initialized");
}

static inline void setActiveScheme(int index) {
  if (index < 0 || index >= NUM_SCHEMES) return;
  _activeScheme = index;
  DEBUG_PRINT("Scheme: "); DEBUG_PRINTLN(SCHEME_NAMES[_activeScheme]);
}

static inline int getActiveScheme() {
  return _activeScheme;
}

static inline void renderActiveScheme() {
  // Intentionally empty, not just pending: the MacroPad never drives the
  // outdoor strips directly (see this file's top comment) — there's
  // nothing for a per-loop MacroPad-side render hook to do. Kept as a
  // no-op rather than removed so the .ino's existing call site doesn't
  // need touching if that ever changes.
}

// ─── Keypad-LED preview swatches ────────────────────────────────────────────
// PROTOTYPE content for the UI/navigation coding example — rough per-month
// color schemes, not the final outdoor-installation design. Tints that are
// specific to one month (not generic/reusable enough for colors.h) live here.
#define PIXEL_DARK_RED      ((uint32_t)0x00640000)  // Color(100,   0,   0)
#define PIXEL_LIGHT_BLUE    ((uint32_t)0x00A0C8FF)  // Color(160, 200, 255)
// April's original pastel values (~25-32% saturation, e.g. 255,182,193)
// read as almost white on the actual LEDs. Bumped to ~50-59% saturation
// below -- still softer/lighter than the fully-saturated primaries used
// elsewhere (e.g. PIXEL_RED at 100% saturation), but now clearly tinted.
#define PIXEL_PASTEL_PINK   ((uint32_t)0x00DC5A8C)  // Color(220,  90, 140)
#define PIXEL_PASTEL_BLUE   ((uint32_t)0x005AA0DC)  // Color( 90, 160, 220)
#define PIXEL_PASTEL_YELLOW ((uint32_t)0x00DCC35A)  // Color(220, 195,  90)
#define PIXEL_PASTEL_GREEN  ((uint32_t)0x0064C878)  // Color(100, 200, 120)

static inline void _fillSolid(uint32_t out[NUM_NEOPIXEL], uint32_t color) {
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) out[i] = color;
}

static inline void _fillCycle(uint32_t out[NUM_NEOPIXEL], const uint32_t* palette, uint8_t count) {
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) out[i] = palette[i % count];
}

// NOT YET VERIFIED: Adafruit_NeoPixel::ColorHSV(hue, sat, val) against the
// installed library version (see CLAUDE.md) — hue is 0-65535 around the
// wheel, returns the same 0x00RRGGBB packing as Color(r,g,b).
static inline void _fillRainbow(uint32_t out[NUM_NEOPIXEL]) {
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    uint16_t hue = (uint32_t)i * 65535 / NUM_NEOPIXEL;
    out[i] = Adafruit_NeoPixel::ColorHSV(hue, 255, 200);
  }
}

// Fills a preview swatch (one color per onboard keypad LED, indices 0-11
// matching keypad_input.h's raw key order) for the given scheme index
// (0-11). See the worked-example month list agreed during brainstorming.
static inline void getSchemePreviewColors(int schemeIndex, uint32_t out[NUM_NEOPIXEL]) {
  switch (schemeIndex) {
    case 0: {  // January — all white
      _fillSolid(out, PIXEL_WHITE);
      break;
    }
    case 1: {  // February — red & white
      static const uint32_t palette[] = { PIXEL_RED, PIXEL_WHITE };
      _fillCycle(out, palette, 2);
      break;
    }
    case 2: {  // March — all green
      _fillSolid(out, PIXEL_GREEN);
      break;
    }
    case 3: {  // April — pastels
      static const uint32_t palette[] = { PIXEL_PASTEL_PINK, PIXEL_PASTEL_BLUE, PIXEL_PASTEL_YELLOW, PIXEL_PASTEL_GREEN };
      _fillCycle(out, palette, 4);
      break;
    }
    case 4: {  // May — Green/Red/White
      static const uint32_t palette[] = { PIXEL_GREEN, PIXEL_RED, PIXEL_WHITE };
      _fillCycle(out, palette, 3);
      break;
    }
    case 5: {  // June — Rainbow
      _fillRainbow(out);
      break;
    }
    case 6: {  // July — Red/White/Blue
      static const uint32_t palette[] = { PIXEL_RED, PIXEL_WHITE, PIXEL_BLUE };
      _fillCycle(out, palette, 3);
      break;
    }
    case 7: {  // August — Red & Orange
      static const uint32_t palette[] = { PIXEL_RED, PIXEL_ORANGE };
      _fillCycle(out, palette, 2);
      break;
    }
    case 8: {  // September — Dark Red & Light Blue
      static const uint32_t palette[] = { PIXEL_DARK_RED, PIXEL_LIGHT_BLUE };
      _fillCycle(out, palette, 2);
      break;
    }
    case 9: {  // October — IMPLEMENTED on the real strips (outdoor_leds.h's
               // showActiveSchemeOnOutdoorLeds()): top strip (A1/12m/2nd
               // floor) purple, bottom strip (A0/18m/1st floor) orange.
               // Mirrored here as top-2-rows/bottom-2-rows (not an
               // alternating cycle), using MacropadShared's
               // MONTHLY_PRESET_OCTOBER_*_COLOR directly (not colors.h's
               // generic PIXEL_PURPLE/PIXEL_ORANGE) so this preview can
               // never drift from what the strips actually show.
      for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
        out[i] = (i < 6) ? MONTHLY_PRESET_OCTOBER_TOP_COLOR : MONTHLY_PRESET_OCTOBER_BOTTOM_COLOR;
      }
      break;
    }
    case 10: {  // November — Orange & Yellow
      static const uint32_t palette[] = { PIXEL_ORANGE, PIXEL_YELLOW };
      _fillCycle(out, palette, 2);
      break;
    }
    case 11: {  // December — Red & Green
      static const uint32_t palette[] = { PIXEL_RED, PIXEL_GREEN };
      _fillCycle(out, palette, 2);
      break;
    }
    default: {
      _fillSolid(out, PIXEL_OFF);
      break;
    }
  }
}
