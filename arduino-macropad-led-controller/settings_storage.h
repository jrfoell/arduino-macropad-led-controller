// settings_storage.h
// Persists the two settings that should survive a power cycle -- the active
// preset/scheme index and the global brightness level -- to the RP2040's
// flash, via the arduino-pico core's built-in EEPROM emulation library
// (bundled with the board core itself, not a separate Library Manager
// install -- see CLAUDE.md's "Expected libraries").
//
// NOT YET VERIFIED against a real install (see CLAUDE.md) -- confirm
// EEPROM.h's begin()/read()/write()/commit() signatures against the
// installed arduino-pico core version before trusting this.
//
// Exposes:
//   initSettingsStorage()                  — call once in setup(), before
//                                             loadSettings()
//   loadSettings(outScheme, outBrightness) — fills in the last-saved values,
//                                             or defaults (scheme 0,
//                                             brightness 100) if nothing was
//                                             ever saved
//   saveSettings(scheme, brightness)       — call on confirm; writes through
//                                             to flash immediately

#pragma once

#include <EEPROM.h>
#include "debug.h"

#define SETTINGS_EEPROM_SIZE 16    // bytes reserved for the emulated EEPROM region
#define SETTINGS_MAGIC       0xB1  // sentinel marking "a save has happened" (erased flash reads 0xFF)

#define SETTINGS_ADDR_MAGIC      0
#define SETTINGS_ADDR_SCHEME     1
#define SETTINGS_ADDR_BRIGHTNESS 2

static inline void initSettingsStorage() {
  EEPROM.begin(SETTINGS_EEPROM_SIZE);
  DEBUG_PRINTLN("Settings storage: initialized");
}

static inline void loadSettings(int *outScheme, int *outBrightness) {
  if (EEPROM.read(SETTINGS_ADDR_MAGIC) == SETTINGS_MAGIC) {
    *outScheme     = EEPROM.read(SETTINGS_ADDR_SCHEME);
    *outBrightness = EEPROM.read(SETTINGS_ADDR_BRIGHTNESS);
    DEBUG_PRINT("Settings storage: loaded scheme="); DEBUG_PRINT(*outScheme);
    DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(*outBrightness);
  } else {
    *outScheme     = 0;
    *outBrightness = 100;
    DEBUG_PRINTLN("Settings storage: no saved settings, using defaults");
  }
}

static inline void saveSettings(int scheme, int brightness) {
  EEPROM.write(SETTINGS_ADDR_MAGIC, SETTINGS_MAGIC);
  EEPROM.write(SETTINGS_ADDR_SCHEME, (uint8_t)scheme);
  EEPROM.write(SETTINGS_ADDR_BRIGHTNESS, (uint8_t)brightness);
  EEPROM.commit();
  DEBUG_PRINT("Settings storage: saved scheme="); DEBUG_PRINT(scheme);
  DEBUG_PRINT(" brightness="); DEBUG_PRINTLN(brightness);
}
