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
//                              Start and the next explicit Stop.
//                              QTPY_CMD_PING (or any other unrecognized
//                              byte) is explicitly ignored here — it only
//                              counts as a heartbeat (see isHostLinkAlive()
//                              below), never as an implicit Stop, so the
//                              MacroPad's once-a-second presence probe
//                              can't interrupt a running Test Pattern.
//   consumeStartPending()    — call every loop(); returns true at most once
//                              per received Start — including a Start
//                              received while already running, so the
//                              phase clock always restarts on a fresh
//                              Start rather than silently continuing a
//                              stale run out of sync with the host
//   isHostLinkAlive()        — true if any QTPY_CMD_PING/START/STOP was
//                              heard within QTPY_LINK_TIMEOUT_MS. There is
//                              no separate heartbeat protocol — this reuses
//                              whatever command traffic already happens
//                              (see i2c_protocol.h). Used by
//                              link_status_led.h.
//   consumeCommandReceivedPending() — call every loop(); returns true at
//                              most once per received Start, Stop, or
//                              SetScheme. QTPY_CMD_PING deliberately never
//                              sets this — it fires roughly once a second
//                              (the MacroPad's presence probe), so counting
//                              it would make the "command received" blink
//                              effectively solid. Used by
//                              link_status_led.h's triggerCommandBlink().
//   consumeSchemeChange(outScheme, outBrightness) — call every loop();
//                              true at most once per received
//                              QTPY_CMD_SET_SCHEME, filling in the 0-11
//                              scheme index and 0-100 brightness it
//                              carried. See scheme_storage.h for what to
//                              do with them (persist + render).
//   consumeBrightnessPreview(outBrightness) — call every loop(); true at
//                              most once per received
//                              QTPY_CMD_PREVIEW_BRIGHTNESS, filling in the
//                              0-100 brightness it carried. Apply
//                              immediately to the current render; never
//                              persist (see i2c_protocol.h).

#pragma once
#include <Wire.h>
#include "i2c_protocol.h"  // from MacropadShared

static volatile bool    _testPatternRunning      = false;
static volatile bool    _startPending            = false;
static volatile bool    _commandReceivedPending  = false;
static volatile bool    _schemeChangePending     = false;
static volatile uint8_t _pendingSchemeIndex      = 0;
static volatile uint8_t _pendingBrightness       = 100;
static volatile bool    _brightnessPreviewPending = false;
static volatile uint8_t _pendingPreviewBrightness = 100;
static volatile unsigned long _lastI2cActivityMs = 0;

static void _onI2cReceive(int numBytes) {
  // NOTE: a bare zero-byte transaction (address only, no Wire.write())
  // never reaches here at all — the installed arduino-pico core's
  // Wire.cpp only invokes this callback `if (_buffLen)`. QTPY_CMD_PING
  // exists specifically so the MacroPad's presence probe always carries a
  // real byte and reliably lands here.
  if (numBytes < 1 || !Wire1.available()) {
    return;
  }
  _lastI2cActivityMs = millis();
  uint8_t cmd = Wire1.read();

  if (cmd == QTPY_CMD_START) {
    _testPatternRunning = true;
    _startPending        = true;  // always re-latch the clock, even if already running
    _commandReceivedPending = true;
  } else if (cmd == QTPY_CMD_STOP) {
    _testPatternRunning = false;
    _commandReceivedPending = true;
  } else if (cmd == QTPY_CMD_SET_SCHEME && Wire1.available() >= 2) {
    _pendingSchemeIndex  = Wire1.read();
    _pendingBrightness   = Wire1.read();
    _schemeChangePending = true;
    _commandReceivedPending = true;
  } else if (cmd == QTPY_CMD_PREVIEW_BRIGHTNESS && Wire1.available() >= 1) {
    _pendingPreviewBrightness = Wire1.read();
    _brightnessPreviewPending = true;
    // Deliberately NOT setting _commandReceivedPending — see
    // i2c_protocol.h: this fires on every encoder detent while the knob
    // turns, which would read as a solid-green glitch, not a blink.
  }
  // QTPY_CMD_PING (and anything else unrecognized, or a malformed payload
  // short on bytes): heartbeat already counted above; deliberately no
  // effect on run state or the command-received blink, so the presence
  // probe can't interrupt a running Test Pattern.

  while (Wire1.available()) {
    Wire1.read();  // drain any unexpected extra bytes
  }
}

static void _onI2cRequest() {
  // Stop is fire-and-forget (see i2c_protocol.h) — the host only ever
  // requests a response after Start, so responding unconditionally here
  // is correct.
  Wire1.write(QTPY_ACK_BYTE);
}

static inline void initI2cCommands() {
  _lastI2cActivityMs = millis();  // grace period before boot could ever read as "timed out"
  Wire1.begin(QTPY_I2C_ADDRESS);
  Wire1.onReceive(_onI2cReceive);
  Wire1.onRequest(_onI2cRequest);
}

static inline bool isHostLinkAlive() {
  return (millis() - _lastI2cActivityMs) < QTPY_LINK_TIMEOUT_MS;
}

static inline bool isTestPatternRunning() {
  return _testPatternRunning;
}

static inline bool consumeStartPending() {
  bool pending = _startPending;
  _startPending = false;
  return pending;
}

static inline bool consumeCommandReceivedPending() {
  bool pending = _commandReceivedPending;
  _commandReceivedPending = false;
  return pending;
}

static inline bool consumeSchemeChange(int* outScheme, int* outBrightness) {
  if (!_schemeChangePending) {
    return false;
  }
  _schemeChangePending = false;
  *outScheme     = _pendingSchemeIndex;
  *outBrightness = _pendingBrightness;
  return true;
}

static inline bool consumeBrightnessPreview(int* outBrightness) {
  if (!_brightnessPreviewPending) {
    return false;
  }
  _brightnessPreviewPending = false;
  *outBrightness = _pendingPreviewBrightness;
  return true;
}
