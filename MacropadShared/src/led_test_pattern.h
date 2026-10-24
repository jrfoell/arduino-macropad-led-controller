// led_test_pattern.h
// Hardware-agnostic "Test Pattern" sequencing logic shared by the MacroPad
// and QT Py sketches — which color/parity should be showing right now,
// and whether a given LED index is lit under that parity. No Arduino
// dependency; each sketch maps this onto its own LEDs (keypad NeoPixels,
// outdoor WS2812 strips) using whatever brightness/color representation
// fits its own hardware.
//
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md
// ("Test Pattern UI") for the full sequence/rationale.
//
// Exposes:
//   LED_TEST_PATTERN_STRIDE             — every-Nth LED lit (2 = every other)
//   LED_TEST_PATTERN_PHASE_MS           — ms per color/parity phase
//   LED_TEST_PATTERN_BRIGHTNESS_PERCENT — fixed brightness (0-100) used
//                                          while Test Pattern is active,
//                                          independent of any other
//                                          brightness setting
//   TestPatternColor, TestPatternPhase  — phase result type
//   testPatternPhaseAt(elapsedMs)       — which color/parity is active
//   testPatternLedIsLit(ledIndex, evenParityLit) — whether this LED is lit

#pragma once
#include <stdint.h>

#define LED_TEST_PATTERN_STRIDE              2
#define LED_TEST_PATTERN_PHASE_MS            5000
#define LED_TEST_PATTERN_BRIGHTNESS_PERCENT  50  // starts conservative; raise toward 100 once wiring is confirmed

enum TestPatternColor { TEST_PATTERN_COLOR_RED, TEST_PATTERN_COLOR_GREEN, TEST_PATTERN_COLOR_BLUE };

struct TestPatternPhase {
  TestPatternColor color;
  bool evenParityLit;
};

// Sequence (repeats every 6*LED_TEST_PATTERN_PHASE_MS): RED-odd, RED-even,
// GREEN-odd, GREEN-even, BLUE-odd, BLUE-even.
static inline TestPatternPhase testPatternPhaseAt(unsigned long elapsedMs) {
  unsigned long phaseIndex = (elapsedMs / LED_TEST_PATTERN_PHASE_MS) % 6;
  TestPatternPhase phase;
  phase.color = (TestPatternColor)(phaseIndex / 2);
  phase.evenParityLit = (phaseIndex % 2) == 1;
  return phase;
}

static inline bool testPatternLedIsLit(uint16_t ledIndex, bool evenParityLit) {
  bool isEvenIndex = (ledIndex % LED_TEST_PATTERN_STRIDE) == 0;
  return isEvenIndex == evenParityLit;
}
