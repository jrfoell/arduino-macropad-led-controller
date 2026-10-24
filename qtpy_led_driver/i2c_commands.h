// i2c_commands.h
// I2C peripheral command handling for the QT Py LED driver — listens on
// Wire1 (the QT Py's STEMMA QT bus, wired to the MacroPad controller) for
// Start/Stop Test Pattern commands (see MacropadShared/src/i2c_protocol.h),
// and reports back an ACK byte on Start so the host can detect a dead link.
//
// The onReceive handler runs in interrupt context (confirmed against the
// installed arduino-pico core's Wire implementation) — it only ever sets
// flags here; `loop()` (see qtpy_led_driver.ino) does the actual rendering,
// clearing, and debug logging, so an I2C interrupt can never preempt a
// NeoPixel `show()`-in-progress or block on a `Serial` write.
//
// Exposes:
//   initI2cCommands()        — call once in setup()
//   isTestPatternRunning()   — call every loop(); true between a received
//                              Start and the next Stop (or any other
//                              non-Start byte)
//   consumeStartPending()    — call every loop(); returns true at most once
//                              per received Start — including a Start
//                              received while already running, so the
//                              phase clock always restarts on a fresh
//                              Start rather than silently continuing a
//                              stale run out of sync with the host

#pragma once
#include <Wire.h>
#include "i2c_protocol.h"  // from MacropadShared

static volatile bool _testPatternRunning = false;
static volatile bool _startPending       = false;

static void _onI2cReceive(int numBytes) {
  if (numBytes < 1 || !Wire1.available()) {
    return;
  }
  uint8_t cmd = Wire1.read();
  while (Wire1.available()) {
    Wire1.read();  // drain any extra bytes
  }

  if (cmd == QTPY_CMD_START) {
    _testPatternRunning = true;
    _startPending        = true;  // always re-latch the clock, even if already running
  } else {
    _testPatternRunning = false;  // any other byte (Stop, or anything unrecognized) stops the run
  }
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
}

static inline bool isTestPatternRunning() {
  return _testPatternRunning;
}

static inline bool consumeStartPending() {
  bool pending = _startPending;
  _startPending = false;
  return pending;
}
