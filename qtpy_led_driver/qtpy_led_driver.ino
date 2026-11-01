// qtpy_led_driver.ino
// Adafruit QT Py RP2040 — I2C peripheral (Wire1/STEMMA QT) that drives the
// two outdoor WS2812B strips on command from the MacroPad controller.
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md

#define DEBUG_SERIAL

#include "debug.h"
#include "outdoor_leds.h"
#include "i2c_commands.h"
#include "link_status_led.h"
#include "scheme_storage.h"

static unsigned long _testPatternStartMs = 0;
static bool _wasRunning = false;
static int  _activeScheme     = 0;
static int  _activeBrightness = 100;

static inline void _showActiveScheme() {
  showActiveSchemeOnOutdoorLeds(_activeScheme, _activeBrightness);
}

void setup() {
  DEBUG_BEGIN(115200);
  initSchemeStorage();
  loadScheme(&_activeScheme, &_activeBrightness);
  initOutdoorLeds();
  initI2cCommands();
  initLinkStatusLed();
  _showActiveScheme();  // the default program at power-up, with no MacroPad involved
  DEBUG_PRINTLN("QT Py LED driver: initialized");
}

void loop() {
  if (consumeCommandReceivedPending()) {
    triggerCommandBlink();
  }
  updateLinkStatusLed(isHostLinkAlive());

  int newScheme, newBrightness;
  if (consumeSchemeChange(&newScheme, &newBrightness)) {
    _activeScheme     = newScheme;
    _activeBrightness = newBrightness;
    saveScheme(_activeScheme, _activeBrightness);
    // Don't fight a running Test Pattern for the strips — in practice this
    // can't happen through the MacroPad's own UI anyway (selecting a
    // Preset always exits Test Pattern, sending Stop, first), but render
    // only once that's actually settled rather than assuming the ordering.
    if (!isTestPatternRunning()) {
      _showActiveScheme();
    }
  }

  bool running = isTestPatternRunning();

  // consumeStartPending() is true on every received Start, including one
  // that arrives while already running — so a restart always re-syncs the
  // phase clock with the host rather than silently continuing a stale run.
  if (consumeStartPending()) {
    _testPatternStartMs = millis();
    DEBUG_PRINTLN("Test Pattern: start (re)received");
  }

  if (!running && _wasRunning) {
    // Only an explicit Stop gets here now (PING no longer implicitly stops
    // a run — see i2c_commands.h). Resume whatever the installation is
    // actually supposed to be showing, rather than going dark.
    DEBUG_PRINTLN("Test Pattern: stopped, resuming active scheme");
    _showActiveScheme();
  }

  if (running) {
    showTestPatternOnOutdoorLeds(millis() - _testPatternStartMs);
  }
  _wasRunning = running;
}
