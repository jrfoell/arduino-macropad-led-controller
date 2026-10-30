// qtpy_link.h
// MacroPad-side (I2C host) link to the QT Py LED driver (I2C peripheral on
// its own Wire1/STEMMA QT bus — see qtpy_led_driver/i2c_commands.h). Sends
// Start/Stop Test Pattern commands (see MacropadShared/src/i2c_protocol.h).
//
// Exposes:
//   initQtpyLink()           — call once in setup()
//   isQtpyConnected()        — a cheap, non-destructive I2C presence probe
//                              — call this whenever the UI wants to show a
//                              "not connected" hint proactively, separate
//                              from actually attempting to start/stop Test
//                              Pattern. The MacroPad works fully without a
//                              QT Py attached either way: this is purely
//                              informational. Writes QTPY_CMD_PING (not a
//                              bare zero-byte transaction) specifically so
//                              it also registers as a heartbeat on the QT
//                              Py's side (see i2c_protocol.h) — a zero-byte
//                              write never reaches the QT Py's onReceive
//                              handler at all on the installed arduino-pico
//                              core, confirmed against that library's own
//                              Wire.cpp.
//   QtpyLinkResult           — startTestPatternOnQtpy()'s result, so a
//                              caller can distinguish *why* it failed (for
//                              logging — ui_controller.h doesn't surface
//                              this as a hint, since Test Pattern runs on
//                              the keypad regardless and isQtpyConnected()
//                              already covers the ambient "not connected"
//                              indicator):
//                                QTPY_LINK_OK           — ACKed, started
//                                QTPY_LINK_NO_RESPONSE  — the transmission
//                                  failed, or nothing came back at all on
//                                  request — likely wiring/power
//                                QTPY_LINK_BAD_ACK      — a response came
//                                  back but didn't match — likely a
//                                  firmware/protocol mismatch, not wiring
//   startTestPatternOnQtpy() — see QtpyLinkResult. On either failure, also
//                              sends Stop first — a Start that reached the
//                              QT Py but whose ACK didn't reach us must not
//                              leave it running unseen.
//   stopTestPatternOnQtpy()  — fire-and-forget, no response expected

#pragma once
#include <Wire.h>
#include "debug.h"
#include "i2c_protocol.h"  // from MacropadShared

enum QtpyLinkResult { QTPY_LINK_OK, QTPY_LINK_NO_RESPONSE, QTPY_LINK_BAD_ACK };

static inline void initQtpyLink() {
  Wire.begin();
  DEBUG_PRINTLN("QT Py link: initialized");
}

// Addresses the QT Py and checks for an ACK. Writes QTPY_CMD_PING, which
// the QT Py's command handler explicitly ignores for run-state purposes
// (see i2c_commands.h) — this never starts/stops/affects anything, unlike
// startTestPatternOnQtpy() and stopTestPatternOnQtpy() below.
static inline bool isQtpyConnected() {
  Wire.beginTransmission(QTPY_I2C_ADDRESS);
  Wire.write(QTPY_CMD_PING);
  return Wire.endTransmission() == 0;
}

static inline void stopTestPatternOnQtpy() {
  Wire.beginTransmission(QTPY_I2C_ADDRESS);
  Wire.write(QTPY_CMD_STOP);
  Wire.endTransmission();
}

static inline QtpyLinkResult startTestPatternOnQtpy() {
  Wire.beginTransmission(QTPY_I2C_ADDRESS);
  Wire.write(QTPY_CMD_START);
  if (Wire.endTransmission() != 0) {
    return QTPY_LINK_NO_RESPONSE;
  }

  Wire.requestFrom(QTPY_I2C_ADDRESS, 1);
  if (Wire.available() < 1) {
    // The QT Py may have already received and acted on the Start even
    // though we can't confirm it (a dropped response, not a dropped Start)
    // — send Stop so a half-succeeded Start can't leave the strips running
    // with no UI anywhere aware of it.
    stopTestPatternOnQtpy();
    return QTPY_LINK_NO_RESPONSE;
  }
  if (Wire.read() != QTPY_ACK_BYTE) {
    stopTestPatternOnQtpy();
    return QTPY_LINK_BAD_ACK;
  }
  return QTPY_LINK_OK;
}
