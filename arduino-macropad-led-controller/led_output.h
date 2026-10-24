// led_output.h
// Drives the outdoor installation's actual display LEDs — the lights the
// audience sees — as opposed to the MacroPad's own onboard NeoPixel
// indicators (colors.h).
//
// STUB: the physical LED type/protocol for the outdoor run has not been
// decided yet (see "Open design questions" in CLAUDE.md — addressable
// strips like WS2812/NeoPixel vs. simple on/off circuits or relays driven
// via the MCP23017 GPIO expander, see gpio_expander.h). Nothing here drives
// real hardware until that decision is made.
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
