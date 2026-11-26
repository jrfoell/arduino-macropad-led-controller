#include "balance.h"
#include <cassert>
#include <cstdio>

static void check(uint8_t brightness, uint8_t balance, uint8_t expTop, uint8_t expBottom) {
  uint8_t top = 255, bottom = 255;
  balanceStripPercents(brightness, balance, &top, &bottom);
  if (top != expTop || bottom != expBottom) {
    printf("FAIL brightness=%u balance=%u -> top=%u bottom=%u (expected %u/%u)\n",
           brightness, balance, top, bottom, expTop, expBottom);
  }
  assert(top == expTop && bottom == expBottom);
}

int main() {
  assert(BALANCE_DEFAULT_PERCENT == 50);
  check(100, 50, 100, 100);  // even
  check(100, 0,  0,   100);  // top strip fully off (maintenance case)
  check(100, 100, 100, 0);   // bottom strip fully off
  check(100, 75, 100, 50);   // right of center dims bottom
  check(100, 25, 50,  100);  // left of center dims top
  check(80,  25, 40,  80);   // composes with brightness, never brightens
  check(80,  75, 80,  40);
  check(0,   30, 0,   0);
  check(100, 255, 100, 0);   // out-of-range balance clamps to 100
  check(100, 5,  10,  100);  // one detent from the top-off end
  printf("test_balance: all passed\n");
  return 0;
}
