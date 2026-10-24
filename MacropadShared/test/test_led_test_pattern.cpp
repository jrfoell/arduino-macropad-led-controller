#include "led_test_pattern.h"
#include <cassert>
#include <climits>
#include <cstdio>

int main() {
  // Phase sequence: RED-odd, RED-even, GREEN-odd, GREEN-even, BLUE-odd, BLUE-even, repeat
  struct { unsigned long ms; TestPatternColor color; bool evenParityLit; } cases[] = {
    {0,     TEST_PATTERN_COLOR_RED,   false},
    {5000,  TEST_PATTERN_COLOR_RED,   true},
    {10000, TEST_PATTERN_COLOR_GREEN, false},
    {15000, TEST_PATTERN_COLOR_GREEN, true},
    {20000, TEST_PATTERN_COLOR_BLUE,  false},
    {25000, TEST_PATTERN_COLOR_BLUE,  true},
    {30000, TEST_PATTERN_COLOR_RED,   false},  // wraps back to phase 0
    {32500, TEST_PATTERN_COLOR_RED,   false},  // mid-phase still holds
  };
  for (auto& c : cases) {
    TestPatternPhase p = testPatternPhaseAt(c.ms);
    assert(p.color == c.color);
    assert(p.evenParityLit == c.evenParityLit);
  }

  // millis() rollover (~49.7 days on the real 32-bit `unsigned long` target
  // — this test host's `unsigned long` may be wider, so pin the actual
  // 32-bit max rather than relying on ULONG_MAX, and check a real value
  // rather than just "doesn't crash."
  TestPatternPhase nearWrap = testPatternPhaseAt(0xFFFFFFFFUL);
  assert(nearWrap.color == TEST_PATTERN_COLOR_GREEN);  // 0xFFFFFFFF/5000 % 6 == 3 -> GREEN-even
  assert(nearWrap.evenParityLit == true);

  // The property that actually matters to callers: `millis() - startMs`
  // (see qtpy_led_driver.ino / ui_controller.h) must keep producing the
  // correct elapsed time across the wrap, via plain unsigned subtraction.
  // Modeled with explicit uint32_t so this host's possibly-wider
  // `unsigned long` doesn't mask the 32-bit wraparound being tested.
  uint32_t startMs         = 0xFFFFFFF0UL;         // 16ms before the wrap
  uint32_t afterWrapMillis = 184UL;                // millis() 200ms later, now wrapped
  uint32_t elapsed         = (uint32_t)(afterWrapMillis - startMs);
  assert(elapsed == 200);
  TestPatternPhase afterWrap = testPatternPhaseAt(elapsed);
  assert(afterWrap.color == TEST_PATTERN_COLOR_RED);  // 200ms in: still the first phase
  assert(afterWrap.evenParityLit == false);

  // Every-other-LED lighting
  assert(testPatternLedIsLit(0, /*evenParityLit=*/true)  == true);
  assert(testPatternLedIsLit(0, /*evenParityLit=*/false) == false);
  assert(testPatternLedIsLit(1, /*evenParityLit=*/true)  == false);
  assert(testPatternLedIsLit(1, /*evenParityLit=*/false) == true);
  assert(testPatternLedIsLit(270, /*evenParityLit=*/true) == true);  // even index, far into an 18m/540-LED strip

  // Render-state change detection (code-review finding: a renderer that
  // skips re-pushing pixels when the phase hasn't changed must forget that
  // memory on a stop/reset, or restarting mid-phase shows stale/dark output
  // for up to one phase duration).
  TestPatternRenderState state;
  testPatternResetRenderState(&state);
  TestPatternPhase redOdd = { TEST_PATTERN_COLOR_RED, false };
  assert(testPatternPhaseChanged(&state, redOdd) == true);   // first call always reports changed
  assert(testPatternPhaseChanged(&state, redOdd) == false);  // same phase again: no change
  testPatternResetRenderState(&state);
  assert(testPatternPhaseChanged(&state, redOdd) == true);   // same phase as before reset: changed again

  printf("All led_test_pattern tests passed\n");
  return 0;
}
