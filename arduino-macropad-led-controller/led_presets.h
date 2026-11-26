// led_presets.h
// Defines the 12 monthly lighting schemes and runs whichever one is active.
// This file is the SOLE authority on what any scheme actually looks like —
// the QT Py has no compiled-in knowledge of "October" or any other month at
// all; on confirm, ui_controller.h builds a scheme's full recipe from here
// (buildSchemeRecipe()) and sends it to the QT Py over I2C (qtpy_link.h's
// setActiveSchemeOnQtpy()), which just stores and renders whatever it's
// told (qtpy_led_driver/outdoor_leds.h + scheme_storage.h). That split
// means tuning a color only ever means editing+reflashing the MacroPad —
// never the QT Py.
//
// Each month is a pair of color palettes (MONTH_PALETTES below): one for
// the top strip (A1/12m/2nd floor), one for the bottom strip (A0/18m/1st
// floor). On the real strips every 3rd LED is lit and the lit LEDs cycle
// through their strip's palette in order (see MacropadShared's
// monthly_presets.h). Most months use the same palette on both strips;
// October is purple-top/orange-bottom.
//
// The keypad-LED preview (getSchemePreviewColors()) is derived from the
// SAME table — top 2 key rows cycle the top palette, bottom 2 rows cycle
// the bottom palette — so the preview can never drift from what the
// strips actually show.
//
// Only October's design is final; the other 11 months' palettes are the
// working drafts agreed during brainstorming and are expected to be tuned
// (see CLAUDE.md's "Open design questions").
//
// Exposes:
//   initSchemes()                       — call once in setup()
//   setActiveScheme(index)              — index 0-11, one per month (0 = January)
//   getActiveScheme()                   — current index
//   renderActiveScheme()                — call every loop(); intentional no-op
//   getSchemePreviewColors(index, out)  — fills out[NUM_NEOPIXEL] with the
//                                          keypad preview for scheme `index`
//   buildSchemeRecipe(index, brightnessPercent, balancePercent, outRecipe)
//                                        — fills in the full SchemeRecipe,
//                                          Brightness x Balance folded into
//                                          one percent per strip
//                                          (MacropadShared's balance.h) (see
//                                          MacropadShared/src/scheme_recipe.h)
//                                          for scheme `index` — exactly what
//                                          gets sent to the QT Py

#pragma once

#include "debug.h"
#include "led_output.h"
#include "colors.h"
#include <scheme_recipe.h>  // from MacropadShared
#include <balance.h>        // from MacropadShared

#define NUM_SCHEMES 12

// TODO: each month's theme — what "significant" event it celebrates — is
// not formally decided yet. Placeholder names only.
static const char* const SCHEME_NAMES[NUM_SCHEMES] = {
  "January",   "February", "March",    "April",
  "May",       "June",     "July",     "August",
  "September", "October",  "November", "December"
};

// ─── Preset-specific tints ──────────────────────────────────────────────────
// Colors tuned for the outdoor strips that aren't generic enough for
// colors.h. Named by color, not month, so they can be reused across presets.
//
// Purple (October top strip) is colors.h's PIXEL_PURPLE, Color(100,0,150),
// used directly — tuning it there also changes June's rainbow purple.

// Color(200,80,0) -> Color(180,50,0) [too close to yellow -> good step
// toward red] -> Color(160,20,0) [same -20R/-30G/+0B delta applied again,
// for a deeper, more saturated red-orange]. Originally October's bottom
// strip; also November's orange. Distinct from colors.h's lighter
// PIXEL_ORANGE (June, August).
#define PIXEL_DEEP_ORANGE   ((uint32_t)0x00A01400)  // Color(160,  20,   0)

#define PIXEL_DARK_RED      ((uint32_t)0x00640000)  // Color(100,   0,   0)
#define PIXEL_LIGHT_BLUE    ((uint32_t)0x00A0C8FF)  // Color(160, 200, 255)
// April's original pastel values (~25-32% saturation, e.g. 255,182,193)
// read as almost white on the keypad LEDs. Bumped to ~50-59% saturation
// below -- still softer/lighter than the fully-saturated primaries used
// elsewhere (e.g. PIXEL_RED at 100% saturation), but now clearly tinted.
#define PIXEL_PASTEL_PINK   ((uint32_t)0x00DC5A8C)  // Color(220,  90, 140)
// Color(220,150,90) was too close to PIXEL_PASTEL_YELLOW outdoors -> less
// green and blue for a deeper orange that still reads lighter than PIXEL_ORANGE.
#define PIXEL_PASTEL_PEACH  ((uint32_t)0x00DC6E3C)  // Color(220, 110,  60)
#define PIXEL_PASTEL_BLUE   ((uint32_t)0x005AA0DC)  // Color( 90, 160, 220)
#define PIXEL_PASTEL_YELLOW ((uint32_t)0x00DCC35A)  // Color(220, 195,  90)
#define PIXEL_PASTEL_GREEN  ((uint32_t)0x0064C878)  // Color(100, 200, 120)
#define PIXEL_PASTEL_PURPLE ((uint32_t)0x00AA64DC)  // Color(170, 100, 220)

// ─── Per-month palettes ─────────────────────────────────────────────────────
// 1 to SCHEME_PALETTE_MAX_COLORS colors each (enforced at compile time by
// the static_assert after MONTH_PALETTES).
static constexpr uint32_t PALETTE_JANUARY[]   = { PIXEL_WHITE };
static constexpr uint32_t PALETTE_FEBRUARY[]  = { PIXEL_RED, PIXEL_WHITE };
static constexpr uint32_t PALETTE_MARCH[]     = { PIXEL_GREEN };
static constexpr uint32_t PALETTE_APRIL[]     = { PIXEL_PASTEL_PINK, PIXEL_PASTEL_PEACH, PIXEL_PASTEL_YELLOW, PIXEL_PASTEL_GREEN, PIXEL_PASTEL_BLUE, PIXEL_PASTEL_PURPLE };  // pastel rainbow, same order as June
static constexpr uint32_t PALETTE_MAY[]       = { PIXEL_GREEN, PIXEL_RED, PIXEL_WHITE };
static constexpr uint32_t PALETTE_JUNE[]      = { PIXEL_RED, PIXEL_ORANGE, PIXEL_YELLOW, PIXEL_GREEN, PIXEL_BLUE, PIXEL_PURPLE };  // rainbow
static constexpr uint32_t PALETTE_JULY[]      = { PIXEL_RED, PIXEL_WHITE, PIXEL_BLUE };
static constexpr uint32_t PALETTE_AUGUST[]    = { PIXEL_RED, PIXEL_ORANGE };
static constexpr uint32_t PALETTE_SEPTEMBER[] = { PIXEL_DARK_RED, PIXEL_LIGHT_BLUE };
static constexpr uint32_t PALETTE_OCTOBER_TOP[]    = { PIXEL_PURPLE };
static constexpr uint32_t PALETTE_OCTOBER_BOTTOM[] = { PIXEL_DEEP_ORANGE };
static constexpr uint32_t PALETTE_NOVEMBER[]  = { PIXEL_DEEP_ORANGE, PIXEL_YELLOW };
static constexpr uint32_t PALETTE_DECEMBER[]  = { PIXEL_RED, PIXEL_GREEN };

struct _MonthPalettes {
  const uint32_t* top;
  uint8_t         topCount;
  const uint32_t* bottom;
  uint8_t         bottomCount;
};

#define _PALETTE_COUNT(p) ((uint8_t)(sizeof(p) / sizeof((p)[0])))
#define _MONTH(top, bottom) { top, _PALETTE_COUNT(top), bottom, _PALETTE_COUNT(bottom) }
#define _MONTH_SAME(p)      _MONTH(p, p)

static constexpr _MonthPalettes MONTH_PALETTES[NUM_SCHEMES] = {
  _MONTH_SAME(PALETTE_JANUARY),
  _MONTH_SAME(PALETTE_FEBRUARY),
  _MONTH_SAME(PALETTE_MARCH),
  _MONTH_SAME(PALETTE_APRIL),
  _MONTH_SAME(PALETTE_MAY),
  _MONTH_SAME(PALETTE_JUNE),
  _MONTH_SAME(PALETTE_JULY),
  _MONTH_SAME(PALETTE_AUGUST),
  _MONTH_SAME(PALETTE_SEPTEMBER),
  _MONTH(PALETTE_OCTOBER_TOP, PALETTE_OCTOBER_BOTTOM),
  _MONTH_SAME(PALETTE_NOVEMBER),
  _MONTH_SAME(PALETTE_DECEMBER),
};

static constexpr bool _monthPalettesFit() {
  for (int i = 0; i < NUM_SCHEMES; i++) {
    const _MonthPalettes& m = MONTH_PALETTES[i];
    if (m.topCount < 1 || m.topCount > SCHEME_PALETTE_MAX_COLORS) return false;
    if (m.bottomCount < 1 || m.bottomCount > SCHEME_PALETTE_MAX_COLORS) return false;
  }
  return true;
}
static_assert(_monthPalettesFit(), "every month palette needs 1..SCHEME_PALETTE_MAX_COLORS colors");

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

static inline void _fillPalette(SchemePalette* out, const uint32_t* colors, uint8_t count) {
  if (count > SCHEME_PALETTE_MAX_COLORS) count = SCHEME_PALETTE_MAX_COLORS;
  out->count = count;
  for (uint8_t i = 0; i < count; i++) out->colors[i] = colors[i];
}

static inline void buildSchemeRecipe(int schemeIndex, int brightnessPercent, int balancePercent, SchemeRecipe* out) {
  out->scheme = (uint8_t)schemeIndex;
  balanceStripPercents((uint8_t)brightnessPercent, (uint8_t)balancePercent, &out->topBrightness, &out->bottomBrightness);
  if (schemeIndex < 0 || schemeIndex >= NUM_SCHEMES) {
    out->top.count    = 0;
    out->bottom.count = 0;
    return;
  }
  const _MonthPalettes& m = MONTH_PALETTES[schemeIndex];
  _fillPalette(&out->top, m.top, m.topCount);
  _fillPalette(&out->bottom, m.bottom, m.bottomCount);
}

// Fills a preview swatch (one color per onboard keypad LED, indices 0-11
// matching keypad_input.h's raw key order — 3 keys per row) for the given
// scheme index (0-11): keys 0-5 (top 2 rows) cycle the top-strip palette,
// keys 6-11 (bottom 2 rows) cycle the bottom-strip palette.
static inline void getSchemePreviewColors(int schemeIndex, uint32_t out[NUM_NEOPIXEL]) {
  if (schemeIndex < 0 || schemeIndex >= NUM_SCHEMES) {
    for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) out[i] = PIXEL_OFF;
    return;
  }
  const _MonthPalettes& m = MONTH_PALETTES[schemeIndex];
  const uint8_t half = NUM_NEOPIXEL / 2;
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    if (i < half) {
      out[i] = m.topCount    ? m.top[i % m.topCount]                 : PIXEL_OFF;
    } else {
      out[i] = m.bottomCount ? m.bottom[(i - half) % m.bottomCount] : PIXEL_OFF;
    }
  }
}
