// qtpy_link.h
// MacroPad-side (I2C host) link to the QT Py LED driver (I2C peripheral on
// its own Wire1/STEMMA QT bus — see qtpy_led_driver/i2c_commands.h). Sends
// Start/Stop Test Pattern commands (see MacropadShared/src/i2c_protocol.h).
//
// Exposes:
//   initQtpyLink()           — call once in setup()
//   startTestPatternOnQtpy() — returns true only if the QT Py ACKed; false
//                              on a failed transmission or a missing/wrong
//                              ACK byte, so the caller can detect a dead
//                              link instead of assuming it started
//   stopTestPatternOnQtpy()  — fire-and-forget, no response expected

#pragma once
#include <Wire.h>
#include "debug.h"
#include "i2c_protocol.h"  // from MacropadShared

static inline void initQtpyLink() {
  Wire.begin();
  DEBUG_PRINTLN("QT Py link: initialized");
}

static inline bool startTestPatternOnQtpy() {
  Wire.beginTransmission(QTPY_I2C_ADDRESS);
  Wire.write(QTPY_CMD_START);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Wire.requestFrom(QTPY_I2C_ADDRESS, 1);
  if (Wire.available() < 1) {
    return false;
  }
  return Wire.read() == QTPY_ACK_BYTE;
}

static inline void stopTestPatternOnQtpy() {
  Wire.beginTransmission(QTPY_I2C_ADDRESS);
  Wire.write(QTPY_CMD_STOP);
  Wire.endTransmission();
}
