// keypad_input.h
// Reads the MacroPad's 12 key switches and reports which one was newly
// pressed, as a raw key index (0-11) — NOT a preset index. Each key is wired
// to its own dedicated GPIO on the RP2040 — this is NOT a scanned matrix, so
// no row/column logic is needed.
//
// KEY_PINS is assumed to be in physical reading order (see the keypad layout
// in docs/superpowers/specs/2026-08-02-ui-navigation-design.md):
//   index:  0  1  2   3  4  5   6  7  8   9    10     11
//   key:    1  2  3   4  5  6   7  8  9   0  <verb>  <noun>
// keyIsDigit()/keyDigitValue() and KEY_INDEX_VERB/KEY_INDEX_NOUN turn a raw
// index into what the verb/noun UI state machine (ui_controller.h) actually
// cares about — the digit value, or which of the two non-numeric keys it is.
//
// Exposes:
//   initKeypad()             — call once in setup()
//   pollKeypad()              — call every loop(); returns the newly-pressed
//                               key's raw index (0-11), or -1 if no new press
//   keyIsDigit(keyIndex)      — true for indices 0-9 (the ten digit keys)
//   keyDigitValue(keyIndex)   — the digit 0-9 a digit key index represents
//   KEY_INDEX_VERB/KEY_INDEX_NOUN — the two non-numeric key indices
//
// KEY_PINS (1-12) is confirmed against Adafruit's official MacroPad RP2040
// pinout diagram (adafruit/Adafruit-MacroPad-RP2040-PCB on GitHub), pulled
// while researching the outdoor LED driver architecture — see
// docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md.

#pragma once

#include "debug.h"

#define NUM_KEYS 12
#define KEY_DEBOUNCE_MS 20

#define KEY_INDEX_VERB 10
#define KEY_INDEX_NOUN 11

static const uint8_t KEY_PINS[NUM_KEYS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

static inline bool keyIsDigit(uint8_t keyIndex) {
  return keyIndex <= 9;
}

// Indices 0-8 are digits 1-9; index 9 is digit 0 (bottom-left key).
static inline uint8_t keyDigitValue(uint8_t keyIndex) {
  return (keyIndex == 9) ? 0 : (keyIndex + 1);
}

static bool          _keyState[NUM_KEYS]       = { false };
static bool          _keyCandidate[NUM_KEYS]   = { false };
static unsigned long _keyCandidateAt[NUM_KEYS] = { 0 };

static inline void initKeypad() {
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    pinMode(KEY_PINS[i], INPUT_PULLUP);
  }
  DEBUG_PRINTLN("Keypad: initialized");
}

// Returns the raw key index (0-11) of a key that just transitioned to
// pressed, debounced, or -1 if nothing new happened this call.
static inline int pollKeypad() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    bool raw = (digitalRead(KEY_PINS[i]) == LOW);  // pressed = LOW with INPUT_PULLUP

    if (raw != _keyCandidate[i]) {
      _keyCandidate[i]   = raw;
      _keyCandidateAt[i] = now;
    }

    if (now - _keyCandidateAt[i] >= KEY_DEBOUNCE_MS && _keyCandidate[i] != _keyState[i]) {
      _keyState[i] = _keyCandidate[i];
      if (_keyState[i]) {
        DEBUG_PRINT_VAL("Key pressed", i);
        return i;
      }
    }
  }
  return -1;
}
