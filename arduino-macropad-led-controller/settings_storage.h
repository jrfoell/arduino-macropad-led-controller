// settings_storage.h
// Persists the settings that should survive a power cycle -- the active
// preset/scheme index, the global brightness level, and the global
// top/bottom Balance (MacropadShared's balance.h) -- to the RP2040's
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
//   loadSettings(outScheme, outBrightness, outBalance) — fills in the
//                                             last-saved values, or defaults
//                                             (scheme 0, brightness 100,
//                                             balance 50 = even) if nothing
//                                             was ever saved. A pre-Balance
//                                             save (SETTINGS_MAGIC_V1) keeps
//                                             its scheme + brightness and
//                                             gets balance 50.
//   saveSettings(scheme, brightness, balance) — call on confirm; writes
//                                             through to flash immediately

#pragma once

#include <EEPROM.h>
#include "debug.h"
#include <balance.h>  // from MacropadShared — BALANCE_DEFAULT_PERCENT

#define SETTINGS_EEPROM_SIZE 16    // bytes reserved for the emulated EEPROM region
#define SETTINGS_MAGIC_V1    0xB1  // old layout: scheme + brightness only (read-only now, for migration)
#define SETTINGS_MAGIC       0xB6  // current layout: + balance. Sentinel marking "a save has happened" (erased flash reads 0xFF)

#define SETTINGS_ADDR_MAGIC      0
#define SETTINGS_ADDR_SCHEME     1
#define SETTINGS_ADDR_BRIGHTNESS 2
#define SETTINGS_ADDR_BALANCE    3

static inline void initSettingsStorage() {
  EEPROM.begin(SETTINGS_EEPROM_SIZE);
  DEBUG_PRINTLN("Settings storage: initialized");
}

static inline void loadSettings(int *outScheme, int *outBrightness, int *outBalance) {
  uint8_t magic = EEPROM.read(SETTINGS_ADDR_MAGIC);
  if (magic == SETTINGS_MAGIC || magic == SETTINGS_MAGIC_V1) {
    *outScheme     = EEPROM.read(SETTINGS_ADDR_SCHEME);
    *outBrightness = EEPROM.read(SETTINGS_ADDR_BRIGHTNESS);
    *outBalance    = (magic == SETTINGS_MAGIC) ? EEPROM.read(SETTINGS_ADDR_BALANCE) : BALANCE_DEFAULT_PERCENT;
    if (*outBalance > 100) *outBalance = BALANCE_DEFAULT_PERCENT;
    DEBUG_PRINT("Settings storage: loaded scheme="); DEBUG_PRINT(*outScheme);
    DEBUG_PRINT(" brightness="); DEBUG_PRINT(*outBrightness);
    DEBUG_PRINT(" balance="); DEBUG_PRINTLN(*outBalance);
  } else {
    *outScheme     = 0;
    *outBrightness = 100;
    *outBalance    = BALANCE_DEFAULT_PERCENT;
    DEBUG_PRINTLN("Settings storage: no saved settings, using defaults");
  }
}

static inline void saveSettings(int scheme, int brightness, int balance) {
  EEPROM.write(SETTINGS_ADDR_MAGIC, SETTINGS_MAGIC);
  EEPROM.write(SETTINGS_ADDR_SCHEME, (uint8_t)scheme);
  EEPROM.write(SETTINGS_ADDR_BRIGHTNESS, (uint8_t)brightness);
  EEPROM.write(SETTINGS_ADDR_BALANCE, (uint8_t)balance);
  EEPROM.commit();
  DEBUG_PRINT("Settings storage: saved scheme="); DEBUG_PRINT(scheme);
  DEBUG_PRINT(" brightness="); DEBUG_PRINT(brightness);
  DEBUG_PRINT(" balance="); DEBUG_PRINTLN(balance);
}
