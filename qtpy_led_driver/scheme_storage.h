// scheme_storage.h
// Persists the QT Py's own active scheme description (index, brightness,
// top/bottom strip colors) to flash via the arduino-pico core's built-in
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
//   loadScheme(outScheme, outBrightness, outTopColor, outBottomColor) —
//                                           fills in the last-saved values,
//                                           or defaults (scheme 0,
//                                           brightness 100, both colors
//                                           off) if nothing was ever saved
//   saveScheme(scheme, brightness, topColor, bottomColor) — call on a
//                                           received QTPY_CMD_SET_SCHEME;
//                                           writes through to flash
//                                           immediately

#pragma once

#include <EEPROM.h>
#include "debug.h"

#define SCHEME_EEPROM_SIZE 16    // bytes reserved for the emulated EEPROM region
#define SCHEME_MAGIC       0xB2  // distinct from the MacroPad's own SETTINGS_MAGIC (0xB1) — separate chips, but keeps the sentinels visually distinguishable in debug logs

#define SCHEME_ADDR_MAGIC      0
#define SCHEME_ADDR_SCHEME     1
#define SCHEME_ADDR_BRIGHTNESS 2
#define SCHEME_ADDR_TOP_R      3
#define SCHEME_ADDR_TOP_G      4
#define SCHEME_ADDR_TOP_B      5
#define SCHEME_ADDR_BOTTOM_R   6
#define SCHEME_ADDR_BOTTOM_G   7
#define SCHEME_ADDR_BOTTOM_B   8

static inline void initSchemeStorage() {
  EEPROM.begin(SCHEME_EEPROM_SIZE);
  DEBUG_PRINTLN("Scheme storage: initialized");
}

static inline void loadScheme(int *outScheme, int *outBrightness, uint32_t *outTopColor, uint32_t *outBottomColor) {
  if (EEPROM.read(SCHEME_ADDR_MAGIC) == SCHEME_MAGIC) {
    *outScheme     = EEPROM.read(SCHEME_ADDR_SCHEME);
    *outBrightness = EEPROM.read(SCHEME_ADDR_BRIGHTNESS);
    uint8_t topR = EEPROM.read(SCHEME_ADDR_TOP_R);
    uint8_t topG = EEPROM.read(SCHEME_ADDR_TOP_G);
    uint8_t topB = EEPROM.read(SCHEME_ADDR_TOP_B);
    uint8_t bottomR = EEPROM.read(SCHEME_ADDR_BOTTOM_R);
    uint8_t bottomG = EEPROM.read(SCHEME_ADDR_BOTTOM_G);
    uint8_t bottomB = EEPROM.read(SCHEME_ADDR_BOTTOM_B);
    *outTopColor    = ((uint32_t)topR << 16)    | ((uint32_t)topG << 8)    | topB;
    *outBottomColor = ((uint32_t)bottomR << 16) | ((uint32_t)bottomG << 8) | bottomB;
    DEBUG_PRINT("Scheme storage: loaded scheme="); DEBUG_PRINT(*outScheme);
    DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(*outBrightness);
  } else {
    *outScheme     = 0;
    *outBrightness = 100;
    *outTopColor    = 0;
    *outBottomColor = 0;
    DEBUG_PRINTLN("Scheme storage: no saved scheme, using defaults");
  }
}

static inline void saveScheme(int scheme, int brightness, uint32_t topColor, uint32_t bottomColor) {
  EEPROM.write(SCHEME_ADDR_MAGIC, SCHEME_MAGIC);
  EEPROM.write(SCHEME_ADDR_SCHEME, (uint8_t)scheme);
  EEPROM.write(SCHEME_ADDR_BRIGHTNESS, (uint8_t)brightness);
  EEPROM.write(SCHEME_ADDR_TOP_R, (uint8_t)(topColor >> 16));
  EEPROM.write(SCHEME_ADDR_TOP_G, (uint8_t)(topColor >> 8));
  EEPROM.write(SCHEME_ADDR_TOP_B, (uint8_t)topColor);
  EEPROM.write(SCHEME_ADDR_BOTTOM_R, (uint8_t)(bottomColor >> 16));
  EEPROM.write(SCHEME_ADDR_BOTTOM_G, (uint8_t)(bottomColor >> 8));
  EEPROM.write(SCHEME_ADDR_BOTTOM_B, (uint8_t)bottomColor);
  EEPROM.commit();
  DEBUG_PRINT("Scheme storage: saved scheme="); DEBUG_PRINT(scheme);
  DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(brightness);
}
