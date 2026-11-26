// scheme_recipe.h
// The full visual "recipe" for one monthly scheme, as the MacroPad
// (instructor) sends it to the QT Py (executor): scheme index, brightness,
// and a small color palette per strip. On the strips, the lit LEDs (every
// MONTHLY_PRESET_LED_STRIDE-th one, see monthly_presets.h) cycle through
// their strip's palette in order, e.g. a 2-color red/white palette lights
// red, (dark, dark), white, (dark, dark), red, ...
//
// Also defines the one fixed-size byte layout used everywhere the recipe
// leaves RAM. Using a single layout for all three means they can't drift:
//   - the QTPY_CMD_SET_SCHEME payload (MacroPad -> QT Py, i2c_protocol.h)
//   - the QTPY_CMD_QUERY_SCHEME response (QT Py -> MacroPad)
//   - the QT Py's own flash copy (qtpy_led_driver/scheme_storage.h)
//
// Packed layout (SCHEME_RECIPE_PACKED_SIZE = 52 bytes):
//   [0] scheme index (0-11)   [1] brightness percent (0-100)
//   [2] top color count       [3] bottom color count
//   [4 .. 4+3*MAX)            top palette, R,G,B per color, unused slots 0
//   [4+3*MAX .. 4+6*MAX)      bottom palette, same
// Well within the arduino-pico core's 256-byte Wire buffer.
//
// Exposes:
//   SCHEME_PALETTE_MAX_COLORS — max colors per strip palette
//   SchemePalette             — { count, colors[] } (0x00RRGGBB colors)
//   SchemeRecipe              — { scheme, brightness, top, bottom }
//   SCHEME_RECIPE_PACKED_SIZE — byte size of the packed layout above
//   packSchemeRecipe(recipe, out)   — recipe -> bytes (unused slots zeroed)
//   unpackSchemeRecipe(in, recipe)  — bytes -> recipe (counts clamped to
//                                     SCHEME_PALETTE_MAX_COLORS, so corrupt
//                                     flash/I2C data can never index past
//                                     the array)

#pragma once
#include <stdint.h>

#define SCHEME_PALETTE_MAX_COLORS 8
#define SCHEME_RECIPE_PACKED_SIZE (4 + 2 * SCHEME_PALETTE_MAX_COLORS * 3)

struct SchemePalette {
  uint8_t  count;
  uint32_t colors[SCHEME_PALETTE_MAX_COLORS];
};

struct SchemeRecipe {
  uint8_t       scheme;
  uint8_t       brightness;
  SchemePalette top;     // A1 / strip B / 12m / 2nd floor
  SchemePalette bottom;  // A0 / strip A / 18m / 1st floor
};

static inline void _packPalette(const SchemePalette* p, uint8_t* out) {
  for (uint8_t i = 0; i < SCHEME_PALETTE_MAX_COLORS; i++) {
    uint32_t c = (i < p->count) ? p->colors[i] : 0;
    out[i * 3]     = (uint8_t)(c >> 16);
    out[i * 3 + 1] = (uint8_t)(c >> 8);
    out[i * 3 + 2] = (uint8_t)c;
  }
}

static inline void _unpackPalette(uint8_t count, const uint8_t* in, SchemePalette* p) {
  p->count = (count > SCHEME_PALETTE_MAX_COLORS) ? SCHEME_PALETTE_MAX_COLORS : count;
  for (uint8_t i = 0; i < SCHEME_PALETTE_MAX_COLORS; i++) {
    p->colors[i] = ((uint32_t)in[i * 3] << 16) | ((uint32_t)in[i * 3 + 1] << 8) | in[i * 3 + 2];
  }
}

static inline void packSchemeRecipe(const SchemeRecipe* r, uint8_t out[SCHEME_RECIPE_PACKED_SIZE]) {
  out[0] = r->scheme;
  out[1] = r->brightness;
  out[2] = (r->top.count > SCHEME_PALETTE_MAX_COLORS) ? SCHEME_PALETTE_MAX_COLORS : r->top.count;
  out[3] = (r->bottom.count > SCHEME_PALETTE_MAX_COLORS) ? SCHEME_PALETTE_MAX_COLORS : r->bottom.count;
  SchemePalette top = r->top, bottom = r->bottom;
  top.count = out[2];
  bottom.count = out[3];
  _packPalette(&top, out + 4);
  _packPalette(&bottom, out + 4 + SCHEME_PALETTE_MAX_COLORS * 3);
}

static inline void unpackSchemeRecipe(const uint8_t in[SCHEME_RECIPE_PACKED_SIZE], SchemeRecipe* r) {
  r->scheme     = in[0];
  r->brightness = in[1];
  _unpackPalette(in[2], in + 4, &r->top);
  _unpackPalette(in[3], in + 4 + SCHEME_PALETTE_MAX_COLORS * 3, &r->bottom);
}
