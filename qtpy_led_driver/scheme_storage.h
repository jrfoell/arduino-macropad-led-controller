// scheme_storage.h
// Persists the QT Py's own active monthly scheme + brightness to flash via
// the arduino-pico core's built-in EEPROM emulation — same mechanism and
// defaults as the MacroPad's own settings_storage.h, but a fully separate
// flash region on a separate chip: the QT Py needs to recall and render
// its last-known scheme immediately at power-up on its own, even if it
// boots before the MacroPad, or without ever hearing from it again (see
// qtpy_link.h's setActiveSchemeOnQtpy() on the MacroPad side, and
// i2c_commands.h's consumeSchemeChange() here).
//
// Exposes:
//   initSchemeStorage()                  — call once in setup(), before
//                                           loadScheme()
//   loadScheme(outScheme, outBrightness) — fills in the last-saved values,
//                                           or defaults (scheme 0,
//                                           brightness 100) if nothing was
//                                           ever saved — same defaults as
//                                           settings_storage.h
//   saveScheme(scheme, brightness)       — call on a received
//                                           QTPY_CMD_SET_SCHEME; writes
//                                           through to flash immediately

#pragma once

#include <EEPROM.h>
#include "debug.h"

#define SCHEME_EEPROM_SIZE 16    // bytes reserved for the emulated EEPROM region
#define SCHEME_MAGIC       0xB2  // distinct from the MacroPad's own SETTINGS_MAGIC (0xB1) — separate chips, but keeps the sentinels visually distinguishable in debug logs

#define SCHEME_ADDR_MAGIC      0
#define SCHEME_ADDR_SCHEME     1
#define SCHEME_ADDR_BRIGHTNESS 2

static inline void initSchemeStorage() {
  EEPROM.begin(SCHEME_EEPROM_SIZE);
  DEBUG_PRINTLN("Scheme storage: initialized");
}

static inline void loadScheme(int *outScheme, int *outBrightness) {
  if (EEPROM.read(SCHEME_ADDR_MAGIC) == SCHEME_MAGIC) {
    *outScheme     = EEPROM.read(SCHEME_ADDR_SCHEME);
    *outBrightness = EEPROM.read(SCHEME_ADDR_BRIGHTNESS);
    DEBUG_PRINT("Scheme storage: loaded scheme="); DEBUG_PRINT(*outScheme);
    DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(*outBrightness);
  } else {
    *outScheme     = 0;
    *outBrightness = 100;
    DEBUG_PRINTLN("Scheme storage: no saved scheme, using defaults");
  }
}

static inline void saveScheme(int scheme, int brightness) {
  EEPROM.write(SCHEME_ADDR_MAGIC, SCHEME_MAGIC);
  EEPROM.write(SCHEME_ADDR_SCHEME, (uint8_t)scheme);
  EEPROM.write(SCHEME_ADDR_BRIGHTNESS, (uint8_t)brightness);
  EEPROM.commit();
  DEBUG_PRINT("Scheme storage: saved scheme="); DEBUG_PRINT(scheme);
  DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(brightness);
}
