// qtpy_led_driver.ino
// Adafruit QT Py RP2040 — I2C peripheral (Wire1/STEMMA QT) that drives the
// two outdoor WS2812B strips on command from the MacroPad controller.
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md

#define DEBUG_SERIAL

#include "debug.h"
#include "outdoor_leds.h"
#include "i2c_commands.h"

static unsigned long _testPatternStartMs = 0;
static bool _wasRunning = false;

void setup() {
  DEBUG_BEGIN(115200);
  initOutdoorLeds();
  initI2cCommands();
  DEBUG_PRINTLN("QT Py LED driver: initialized");
}

void loop() {
  bool running = isTestPatternRunning();

  // consumeStartPending() is true on every received Start, including one
  // that arrives while already running — so a restart always re-syncs the
  // phase clock with the host rather than silently continuing a stale run.
  if (consumeStartPending()) {
    _testPatternStartMs = millis();
    DEBUG_PRINTLN("Test Pattern: start (re)received");
  }

  if (!running && _wasRunning) {
    // Covers an explicit Stop and any unrecognized command byte alike —
    // either way, "not running" is what should blank the strips.
    clearOutdoorLeds();
    DEBUG_PRINTLN("Test Pattern: stopped");
  }

  if (running) {
    showTestPatternOnOutdoorLeds(millis() - _testPatternStartMs);
  }
  _wasRunning = running;
}
