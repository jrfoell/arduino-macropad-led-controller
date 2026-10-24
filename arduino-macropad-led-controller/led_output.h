// led_output.h
// Drives the outdoor installation's actual display LEDs — the lights the
// audience sees — as opposed to the MacroPad's own onboard NeoPixel
// indicators (colors.h).
//
// STUB: the LED type/protocol question is now decided — WS2812B strips,
// driven by a QT Py RP2040 companion controller over I2C (see
// docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md)
// rather than anything on this board's own GPIO. That companion
// controller's "Test Pattern" diagnostic mode is implemented
// (see qtpy_link.h, ui_controller.h); per-scheme rendering of the 12
// monthly presets onto the real strips is still unbuilt, pending each
// month's visual design (see CLAUDE.md's "Open design questions").
//
// Exposes (planned):
//   initLedOutput()  — call once in setup()
//   renderFrame(...) — call from loop() with whatever state the active
//                      scheme (led_presets.h) needs rendered

#pragma once

#include "debug.h"

static inline void initLedOutput() {
  DEBUG_PRINTLN("LED output: not yet implemented (LED type undecided)");
}
