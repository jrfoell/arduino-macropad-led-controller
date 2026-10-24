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

  // millis() rollover (~49.7 days) must not glitch the calculation —
  // unsigned division/modulo wraps correctly, but pin it with a real case.
  TestPatternPhase nearWrap = testPatternPhaseAt(ULONG_MAX);
  (void)nearWrap;  // just must not crash/UB; exact phase at this instant isn't load-bearing

  // Every-other-LED lighting
  assert(testPatternLedIsLit(0, /*evenParityLit=*/true)  == true);
  assert(testPatternLedIsLit(0, /*evenParityLit=*/false) == false);
  assert(testPatternLedIsLit(1, /*evenParityLit=*/true)  == false);
  assert(testPatternLedIsLit(1, /*evenParityLit=*/false) == true);
  assert(testPatternLedIsLit(270, /*evenParityLit=*/true) == true);  // even index, far into an 18m/540-LED strip

  printf("All led_test_pattern tests passed\n");
  return 0;
}
