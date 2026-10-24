// i2c_commands.h
// I2C peripheral command handling for the QT Py LED driver — listens on
// Wire1 (the QT Py's STEMMA QT bus, wired to the MacroPad controller) for
// Start/Stop Test Pattern commands (see MacropadShared/src/i2c_protocol.h),
// and reports back an ACK byte on Start so the host can detect a dead link.
//
// Exposes:
//   initI2cCommands()      — call once in setup()
//   isTestPatternRunning() — call every loop(); true between a received
//                            Start and the next Stop

#pragma once
#include <Wire.h>
#include "debug.h"
#include "outdoor_leds.h"
#include "i2c_protocol.h"  // from MacropadShared

static volatile bool _testPatternRunning = false;

static void _onI2cReceive(int numBytes) {
  if (numBytes < 1 || !Wire1.available()) {
    return;
  }
  uint8_t cmd = Wire1.read();
  while (Wire1.available()) {
    Wire1.read();  // drain any extra bytes
  }

  _testPatternRunning = (cmd == QTPY_CMD_START);
  if (cmd == QTPY_CMD_STOP) {
    clearOutdoorLeds();
  }
  DEBUG_PRINT_VAL("I2C command received", cmd);
}

static void _onI2cRequest() {
  // Stop is fire-and-forget (see i2c_protocol.h) — the host only ever
  // requests a response after Start, so responding unconditionally here
  // is correct.
  Wire1.write(QTPY_ACK_BYTE);
}

static inline void initI2cCommands() {
  Wire1.begin(QTPY_I2C_ADDRESS);
  Wire1.onReceive(_onI2cReceive);
  Wire1.onRequest(_onI2cRequest);
  DEBUG_PRINTLN("I2C commands: initialized");
}

static inline bool isTestPatternRunning() {
  return _testPatternRunning;
}
