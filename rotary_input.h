// rotary_input.h
// The MacroPad's rotary encoder (20 detents/360° rotation — see CLAUDE.md and
// docs/superpowers/specs/2026-08-02-ui-navigation-design.md) and its
// push-button switch. Used by ui_controller.h to scroll/confirm the OLED
// menu and to fine-tune a picked color/pattern.
//
// Quadrature decoding is delegated to the third-party RotaryEncoder library
// (the same one used in Adafruit's own Arduino example for this board,
// learn.adafruit.com/adafruit-macropad-rp2040/arduino), configured with
// `LatchMode::FOUR3` to match that example. This is also what accounts for
// this rotary encoder's detents not lining up 1:1 with a single raw
// quadrature transition (see "Encoder detent behavior" in the design doc) —
// the library handles that internally, so pollRotaryDelta() doesn't need
// its own accumulate-by-2 workaround anymore.
//
// Exposes:
//   initRotary()       — call once in setup()
//   pollRotaryDelta()  — call every loop(); returns detent movement since
//                        last call (+1/-1 per felt detent, 0 if none)
//   rotaryPressed()    — true once per debounced button press
//
// PIN_ROTA / PIN_ROTB / PIN_SWITCH come from the "Adafruit MacroPad RP2040"
// board variant — confirm they're actually defined once the arduino-pico
// core is installed (not yet verified against the real header). PIN_SWITCH
// also doubles as the BOOT-select button when held at power-up, so avoid
// depending on it for anything power-on-critical.
//
// Felt direction is inverted from the library's raw position count (see
// ROTARY_DIRECTION below) — flip it back to 1 if a future rewiring/library
// change reverses which way feels "forward" again.

#pragma once

#include <RotaryEncoder.h>
#include "debug.h"

#define ROTARY_SWITCH_DEBOUNCE_MS 30

// -1 reverses the felt direction of every menu-scroll/brightness rotation
// relative to the library's raw getPosition() count; 1 would match it as-is.
#define ROTARY_DIRECTION -1

static RotaryEncoder rotaryEncoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
static long _rotaryLastPosition = 0;

static bool          _rotarySwitchState       = false;
static bool          _rotarySwitchCandidate   = false;
static unsigned long _rotarySwitchCandidateAt = 0;

// Mirrors Adafruit's own example: an interrupt on each pin keeps tick()
// current between loop() iterations, so a fast spin can't miss a detent.
// This is the only interrupt-driven code in the sketch.
static inline void _rotaryCheckPosition() {
  rotaryEncoder.tick();
}

static inline void initRotary() {
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), _rotaryCheckPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), _rotaryCheckPosition, CHANGE);
  DEBUG_PRINTLN("Rotary: initialized");
}

static inline int pollRotaryDelta() {
  rotaryEncoder.tick();  // belt-and-suspenders alongside the ISR
  long newPosition = rotaryEncoder.getPosition();
  int delta = (int)(newPosition - _rotaryLastPosition);
  _rotaryLastPosition = newPosition;
  return delta * ROTARY_DIRECTION;
}

static inline bool rotaryPressed() {
  unsigned long now = millis();
  bool raw = (digitalRead(PIN_SWITCH) == LOW);

  if (raw != _rotarySwitchCandidate) {
    _rotarySwitchCandidate   = raw;
    _rotarySwitchCandidateAt = now;
  }

  if (now - _rotarySwitchCandidateAt >= ROTARY_SWITCH_DEBOUNCE_MS && _rotarySwitchCandidate != _rotarySwitchState) {
    _rotarySwitchState = _rotarySwitchCandidate;
    if (_rotarySwitchState) {
      DEBUG_PRINTLN("Rotary: button pressed");
      return true;
    }
  }
  return false;
}
