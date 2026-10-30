// link_status_led.h
// Drives the QT Py's own onboard NeoPixel (its single built-in status LED
// — distinct from outdoor_leds.h's two outdoor WS2812B strips) as a host-
// link indicator: solid red whenever i2c_commands.h's isHostLinkAlive()
// reports no I2C contact from the MacroPad within QTPY_LINK_TIMEOUT_MS
// (STEMMA QT cable unplugged, MacroPad powered off, etc.), off otherwise.
//
// NEOPIXEL_POWER (GPIO11) is wired directly to the onboard NeoPixel's VDD
// pin on this board — confirmed against Adafruit's own schematic
// (adafruit/Adafruit-QT-Py-RP2040-PCB), no switching FET in between — so
// it must be driven HIGH to power the LED at all, unlike boards where
// NEOPIXEL_POWER_ON/OFF constants exist for an inverted FET-switched rail.
//
// Exposes:
//   initLinkStatusLed()            — call once in setup()
//   updateLinkStatusLed(linkAlive) — call every loop()

#pragma once
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel linkStatusLed(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

static bool _linkStatusLedLastAlive = true;
static bool _linkStatusLedHaveState = false;

static inline void initLinkStatusLed() {
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
  linkStatusLed.begin();
  linkStatusLed.show();
}

static inline void updateLinkStatusLed(bool linkAlive) {
  if (_linkStatusLedHaveState && linkAlive == _linkStatusLedLastAlive) {
    return;
  }
  linkStatusLed.setPixelColor(0, linkAlive ? 0 : linkStatusLed.Color(255, 0, 0));
  linkStatusLed.show();
  _linkStatusLedLastAlive = linkAlive;
  _linkStatusLedHaveState = true;
}
