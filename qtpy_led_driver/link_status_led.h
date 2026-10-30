// link_status_led.h
// Drives the QT Py's own onboard NeoPixel (its single built-in status LED
// — distinct from outdoor_leds.h's two outdoor WS2812B strips) as a host-
// link indicator, in priority order:
//   1. Green, for COMMAND_BLINK_MS, right after a real command (Start or
//      Stop — not the PING heartbeat) was received — see
//      i2c_commands.h's consumeCommandReceivedPending().
//   2. Red, whenever i2c_commands.h's isHostLinkAlive() reports no I2C
//      contact from the MacroPad within QTPY_LINK_TIMEOUT_MS (STEMMA QT
//      cable unplugged, MacroPad powered off, etc.).
//   3. Off, otherwise (connected, idle).
//
// NEOPIXEL_POWER (GPIO11) is wired directly to the onboard NeoPixel's VDD
// pin on this board — confirmed against Adafruit's own schematic
// (adafruit/Adafruit-QT-Py-RP2040-PCB), no switching FET in between — so
// it must be driven HIGH to power the LED at all, unlike boards where
// NEOPIXEL_POWER_ON/OFF constants exist for an inverted FET-switched rail.
//
// Exposes:
//   initLinkStatusLed()            — call once in setup()
//   triggerCommandBlink()          — call from loop() (never from the I2C
//                                     ISR — see i2c_commands.h) when
//                                     consumeCommandReceivedPending() was
//                                     true; starts the green flash
//   updateLinkStatusLed(linkAlive) — call every loop(), after any
//                                     triggerCommandBlink() call so a
//                                     fresh blink wins this same iteration

#pragma once
#include <Adafruit_NeoPixel.h>

#define COMMAND_BLINK_MS 150  // how long the green "command received" flash lasts

static Adafruit_NeoPixel linkStatusLed(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

enum LinkStatusLedState { LINK_STATUS_LED_UNKNOWN = -1, LINK_STATUS_LED_OFF, LINK_STATUS_LED_RED, LINK_STATUS_LED_GREEN };

static int _linkStatusLedLastShown = LINK_STATUS_LED_UNKNOWN;
static unsigned long _commandBlinkUntilMs = 0;

static inline void initLinkStatusLed() {
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
  linkStatusLed.begin();
  linkStatusLed.show();
}

static inline void triggerCommandBlink() {
  _commandBlinkUntilMs = millis() + COMMAND_BLINK_MS;
}

static inline void updateLinkStatusLed(bool linkAlive) {
  // Signed subtraction so a millis() rollover mid-blink still compares
  // correctly, same idiom as MacropadShared's test for the equivalent case.
  bool blinking = (long)(millis() - _commandBlinkUntilMs) < 0;
  LinkStatusLedState desired = blinking ? LINK_STATUS_LED_GREEN
                             : linkAlive ? LINK_STATUS_LED_OFF
                             :             LINK_STATUS_LED_RED;
  if (desired == _linkStatusLedLastShown) {
    return;
  }
  uint32_t color = (desired == LINK_STATUS_LED_GREEN) ? linkStatusLed.Color(0, 255, 0)
                 : (desired == LINK_STATUS_LED_RED)   ? linkStatusLed.Color(255, 0, 0)
                 :                                       0;
  linkStatusLed.setPixelColor(0, color);
  linkStatusLed.show();
  _linkStatusLedLastShown = desired;
}
