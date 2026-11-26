// scheme_storage.h
// Persists the QT Py's own active scheme description (index, brightness,
// top/bottom strip color palettes) to flash via the arduino-pico core's built-in
// EEPROM emulation — same mechanism as the MacroPad's own
// settings_storage.h, but a fully separate flash region on a separate
// chip: the QT Py needs to recall and render exactly what it was last told
// immediately at power-up on its own, even if it boots before the
// MacroPad, or without ever hearing from it again. The QT Py has no
// compiled-in knowledge of what any scheme "means" — it just remembers the
// last recipe the MacroPad sent (see qtpy_link.h's setActiveSchemeOnQtpy()
// on the MacroPad side, and i2c_commands.h's consumeSchemeChange() here).
//
// Exposes:
//   initSchemeStorage()                  — call once in setup(), before
//                                           loadScheme()
//   loadScheme(outRecipe)                — fills in the last-saved
//                                           SchemeRecipe (see
//                                           MacropadShared/src/scheme_recipe.h),
//                                           or defaults (scheme 0, both
//                                           strips at 100%, both palettes
//                                           empty = off) if nothing was
//                                           ever saved. Also migrates both
//                                           older layouts — V1
//                                           (single color per strip) and V2
//                                           (palettes, one shared
//                                           brightness) — so a QT Py
//                                           flashed with this version keeps
//                                           showing what it showed before,
//                                           until the MacroPad next sends a
//                                           scheme.
//   saveScheme(recipe)                   — call on a received
//                                           QTPY_CMD_SET_SCHEME; writes
//                                           through to flash immediately
//   loadAndAdvanceLedOffset(stride)      — call once in setup(), after
//                                           initSchemeStorage(): returns
//                                           the LED-pattern offset
//                                           (0..stride-1) to use THIS boot,
//                                           and persists the next one
//                                           (current+1 mod stride) so the
//                                           lit LEDs shift by one each
//                                           power-up — see
//                                           MacropadShared/src/monthly_presets.h's
//                                           monthlyPresetLedIsLit(). Needs
//                                           no magic-byte/"ever saved"
//                                           check unlike the scheme fields
//                                           above: any raw byte, including
//                                           fresh/erased flash, reduces to
//                                           a valid offset via `% stride`.

#pragma once

#include <EEPROM.h>
#include "debug.h"
#include "scheme_recipe.h"  // from MacropadShared

// 128 rather than the old 16: the recipe blob now starts at byte 16 and is
// SCHEME_RECIPE_PACKED_SIZE (52) bytes. Growing the size is safe for
// existing data — the core's EEPROM.begin() copies the first `size` bytes
// of the same flash sector, so addresses 0-15 read back exactly as before.
#define SCHEME_EEPROM_SIZE 128   // bytes reserved for the emulated EEPROM region
#define SCHEME_MAGIC_V1    0xB2  // old layout: one color per strip at addrs 1-8 (read-only now, for migration)
#define SCHEME_MAGIC_V2    0xB3  // old layout: 52-byte recipe at SCHEME_ADDR_RECIPE with ONE brightness byte (read-only now, for migration)
#define SCHEME_MAGIC       0xB4  // current layout: packed 53-byte SchemeRecipe (per-strip brightness) at SCHEME_ADDR_RECIPE. Distinct from the MacroPad's own SETTINGS_MAGIC (0xB1) — separate chips, but keeps the sentinels visually distinguishable in debug logs

#define SCHEME_ADDR_MAGIC      0
// V1-only fields (addrs 1-8), kept so an old save can be migrated:
#define SCHEME_ADDR_V1_SCHEME     1
#define SCHEME_ADDR_V1_BRIGHTNESS 2
#define SCHEME_ADDR_V1_TOP_RGB    3  // 3 bytes: R, G, B
#define SCHEME_ADDR_V1_BOTTOM_RGB 6  // 3 bytes: R, G, B
#define SCHEME_ADDR_LED_OFFSET 9     // unchanged across layouts
#define SCHEME_ADDR_RECIPE     16    // SCHEME_RECIPE_PACKED_SIZE bytes

static inline void initSchemeStorage() {
  EEPROM.begin(SCHEME_EEPROM_SIZE);
  DEBUG_PRINTLN("Scheme storage: initialized");
}

static inline uint32_t _readV1Color(int addr) {
  return ((uint32_t)EEPROM.read(addr) << 16) | ((uint32_t)EEPROM.read(addr + 1) << 8) | EEPROM.read(addr + 2);
}

static inline void loadScheme(SchemeRecipe* outRecipe) {
  uint8_t magic = EEPROM.read(SCHEME_ADDR_MAGIC);
  if (magic == SCHEME_MAGIC) {
    uint8_t packed[SCHEME_RECIPE_PACKED_SIZE];
    for (uint8_t i = 0; i < SCHEME_RECIPE_PACKED_SIZE; i++) {
      packed[i] = EEPROM.read(SCHEME_ADDR_RECIPE + i);
    }
    unpackSchemeRecipe(packed, outRecipe);
    DEBUG_PRINT("Scheme storage: loaded scheme="); DEBUG_PRINT(outRecipe->scheme);
  } else if (magic == SCHEME_MAGIC_V2) {
    // V2 = today's layout minus the bottom-brightness byte: [0] scheme
    // [1] brightness [2..] counts + palettes. Rebuild it in the current
    // layout with that one brightness applied to both strips.
    uint8_t packed[SCHEME_RECIPE_PACKED_SIZE];
    packed[0] = EEPROM.read(SCHEME_ADDR_RECIPE);
    packed[1] = EEPROM.read(SCHEME_ADDR_RECIPE + 1);
    packed[2] = packed[1];
    for (uint8_t i = 3; i < SCHEME_RECIPE_PACKED_SIZE; i++) {
      packed[i] = EEPROM.read(SCHEME_ADDR_RECIPE + i - 1);
    }
    unpackSchemeRecipe(packed, outRecipe);
    DEBUG_PRINT("Scheme storage: migrated V2 scheme="); DEBUG_PRINT(outRecipe->scheme);
  } else if (magic == SCHEME_MAGIC_V1) {
    outRecipe->scheme           = EEPROM.read(SCHEME_ADDR_V1_SCHEME);
    outRecipe->topBrightness    = EEPROM.read(SCHEME_ADDR_V1_BRIGHTNESS);
    outRecipe->bottomBrightness = outRecipe->topBrightness;
    outRecipe->top.count        = 1;
    outRecipe->top.colors[0]    = _readV1Color(SCHEME_ADDR_V1_TOP_RGB);
    outRecipe->bottom.count     = 1;
    outRecipe->bottom.colors[0] = _readV1Color(SCHEME_ADDR_V1_BOTTOM_RGB);
    DEBUG_PRINT("Scheme storage: migrated V1 scheme="); DEBUG_PRINT(outRecipe->scheme);
  } else {
    outRecipe->scheme           = 0;
    outRecipe->topBrightness    = 100;
    outRecipe->bottomBrightness = 100;
    outRecipe->top.count    = 0;
    outRecipe->bottom.count = 0;
    DEBUG_PRINTLN("Scheme storage: no saved scheme, using defaults");
    return;
  }
  DEBUG_PRINT(" top%="); DEBUG_PRINT(outRecipe->topBrightness);
  DEBUG_PRINT(" bottom%="); DEBUG_PRINT(outRecipe->bottomBrightness);
  DEBUG_PRINT(" topColors="); DEBUG_PRINT(outRecipe->top.count);
  DEBUG_PRINT(" bottomColors="); DEBUG_PRINTLN(outRecipe->bottom.count);
}

static inline void saveScheme(const SchemeRecipe* recipe) {
  uint8_t packed[SCHEME_RECIPE_PACKED_SIZE];
  packSchemeRecipe(recipe, packed);
  for (uint8_t i = 0; i < SCHEME_RECIPE_PACKED_SIZE; i++) {
    EEPROM.write(SCHEME_ADDR_RECIPE + i, packed[i]);
  }
  EEPROM.write(SCHEME_ADDR_MAGIC, SCHEME_MAGIC);
  EEPROM.commit();
  DEBUG_PRINT("Scheme storage: saved scheme="); DEBUG_PRINT(recipe->scheme);
  DEBUG_PRINT(" top%="); DEBUG_PRINT(recipe->topBrightness);
  DEBUG_PRINT(" bottom%="); DEBUG_PRINTLN(recipe->bottomBrightness);
}

static inline uint8_t loadAndAdvanceLedOffset(uint8_t stride) {
  uint8_t offset = EEPROM.read(SCHEME_ADDR_LED_OFFSET) % stride;
  EEPROM.write(SCHEME_ADDR_LED_OFFSET, (offset + 1) % stride);
  EEPROM.commit();
  DEBUG_PRINT("Scheme storage: LED offset this boot="); DEBUG_PRINTLN(offset);
  return offset;
}
