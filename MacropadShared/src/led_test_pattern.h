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
//   TestPatternRenderState              — tracks the last-rendered phase so
//                                          a renderer can skip redundant
//                                          pixel pushes
//   testPatternResetRenderState(state)  — call when (re)starting a render
//                                          loop or blanking the LEDs, so the
//                                          next phase is always reported as
//                                          "changed" rather than silently
//                                          matching stale memory from a
//                                          previous run
//   testPatternPhaseChanged(state, phase) — true (and updates state) iff
//                                          `phase` differs from the last one
//                                          seen since the last reset

#pragma once
#include <stdint.h>
#include <assert.h>

#define LED_TEST_PATTERN_STRIDE              2
#define LED_TEST_PATTERN_PHASE_MS            5000
#define LED_TEST_PATTERN_BRIGHTNESS_PERCENT  50  // starts conservative; raise toward 100 once wiring is confirmed

// testPatternPhaseAt()/testPatternLedIsLit() hardcode exactly two parities
// and the Test Pattern power budget (see the design doc) is computed
// assuming every-other lighting — raising this would silently light more
// LEDs per phase than that math accounts for. Not freely retunable; this
// guard turns a future "just bump the constant" into a build error instead
// of a field over-current surprise.
static_assert(LED_TEST_PATTERN_STRIDE == 2, "LED_TEST_PATTERN_STRIDE must stay 2 -- see the comment above it");

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

struct TestPatternRenderState {
  TestPatternPhase lastPhase;
  bool havePhase;
};

// Call on (re)starting a render loop or blanking the LEDs. Without this, a
// renderer's own stale "last phase" memory can make a restart mid-phase
// look like "nothing changed" and skip re-pushing pixels, leaving the LEDs
// dark/stale for up to one phase duration.
static inline void testPatternResetRenderState(TestPatternRenderState* state) {
  state->havePhase = false;
}

static inline bool testPatternPhaseChanged(TestPatternRenderState* state, TestPatternPhase phase) {
  bool changed = !state->havePhase
    || phase.color != state->lastPhase.color
    || phase.evenParityLit != state->lastPhase.evenParityLit;
  state->lastPhase = phase;
  state->havePhase = true;
  return changed;
}
