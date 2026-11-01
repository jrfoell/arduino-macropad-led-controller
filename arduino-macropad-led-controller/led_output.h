// led_output.h
// Drives the outdoor installation's actual display LEDs — the lights the
// audience sees — as opposed to the MacroPad's own onboard NeoPixel
// indicators (colors.h).
//
// STUB — and likely permanently so: the LED type/protocol question is
// decided (WS2812B strips, driven by a QT Py RP2040 companion controller
// over I2C — see
// docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md),
// and it turns out the MacroPad never drives the outdoor strips directly
// at all, in any scenario — not even for per-scheme rendering. On a
// preset confirm, ui_controller.h just sends the scheme index + brightness
// to the QT Py over I2C (qtpy_link.h's setActiveSchemeOnQtpy()), which does
// its own rendering AND its own flash persistence independently
// (qtpy_led_driver/outdoor_leds.h + scheme_storage.h) — including
// restoring and re-rendering its last scheme at its own power-up with no
// MacroPad involved. October is implemented this way already; the other
// 11 months are still pending each month's visual design (see CLAUDE.md's
// "Open design questions"). This file is kept as a harmless init no-op in
// case a real MacroPad-side use ever materializes, not because one is
// expected.
//
// Exposes:
//   initLedOutput()  — call once in setup()

#pragma once

#include "debug.h"

static inline void initLedOutput() {
  DEBUG_PRINTLN("LED output: no-op — outdoor rendering happens on the QT Py, not here");
}
