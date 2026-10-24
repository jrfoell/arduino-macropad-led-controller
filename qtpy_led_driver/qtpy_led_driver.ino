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
  if (running && !_wasRunning) {
    _testPatternStartMs = millis();
  }
  if (running) {
    showTestPatternOnOutdoorLeds(millis() - _testPatternStartMs);
  }
  _wasRunning = running;
}
