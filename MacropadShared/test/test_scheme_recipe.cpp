#include "scheme_recipe.h"
#include "monthly_presets.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
  // ─── pack/unpack round trip ───────────────────────────────────────────────
  SchemeRecipe r;
  memset(&r, 0xEE, sizeof(r));  // garbage beyond `count` must not leak into the packed bytes
  r.scheme     = 6;
  r.topBrightness    = 75;
  r.bottomBrightness = 40;
  r.top.count  = 3;
  r.top.colors[0] = 0x00C80000; r.top.colors[1] = 0x00646464; r.top.colors[2] = 0x000000C8;
  r.bottom.count = 1;
  r.bottom.colors[0] = 0x00A01400;

  uint8_t buf[SCHEME_RECIPE_PACKED_SIZE];
  packSchemeRecipe(&r, buf);
  assert(SCHEME_RECIPE_PACKED_SIZE == 5 + 2 * SCHEME_PALETTE_MAX_COLORS * 3);
  assert(buf[0] == 6 && buf[1] == 75 && buf[2] == 40 && buf[3] == 3 && buf[4] == 1);
  assert(buf[5] == 0xC8 && buf[6] == 0x00 && buf[7] == 0x00);  // top[0] R,G,B
  assert(buf[8] == 0x64 && buf[9] == 0x64 && buf[10] == 0x64);  // top[1]
  // Unused top slots are zeroed, not garbage
  for (int i = 5 + 3 * 3; i < 5 + SCHEME_PALETTE_MAX_COLORS * 3; i++) assert(buf[i] == 0);
  int bottomBase = 5 + SCHEME_PALETTE_MAX_COLORS * 3;
  assert(buf[bottomBase] == 0xA0 && buf[bottomBase + 1] == 0x14 && buf[bottomBase + 2] == 0x00);

  SchemeRecipe back;
  unpackSchemeRecipe(buf, &back);
  assert(back.scheme == 6 && back.topBrightness == 75 && back.bottomBrightness == 40);
  assert(back.top.count == 3 && back.bottom.count == 1);
  assert(back.top.colors[0] == 0x00C80000);
  assert(back.top.colors[1] == 0x00646464);
  assert(back.top.colors[2] == 0x000000C8);
  assert(back.bottom.colors[0] == 0x00A01400);

  // A corrupt/oversized count is clamped rather than indexing past the array
  buf[3] = 200;
  unpackSchemeRecipe(buf, &back);
  assert(back.top.count == SCHEME_PALETTE_MAX_COLORS);

  // ─── palette color at each LED, every 3rd LED lit ─────────────────────────
  SchemePalette p;
  p.count = 2;
  p.colors[0] = 0x00C80000;  // red
  p.colors[1] = 0x00646464;  // white

  // offset 0: LEDs 0,3,6,9 lit -> red, white, red, white
  assert(monthlyPresetPaletteColorAt(0, 0, &p) == 0x00C80000);
  assert(monthlyPresetPaletteColorAt(1, 0, &p) == 0);
  assert(monthlyPresetPaletteColorAt(2, 0, &p) == 0);
  assert(monthlyPresetPaletteColorAt(3, 0, &p) == 0x00646464);
  assert(monthlyPresetPaletteColorAt(6, 0, &p) == 0x00C80000);
  assert(monthlyPresetPaletteColorAt(9, 0, &p) == 0x00646464);

  // offset 2: LEDs 2,5,8 lit -> still alternates red, white, red
  assert(monthlyPresetPaletteColorAt(0, 2, &p) == 0);
  assert(monthlyPresetPaletteColorAt(2, 2, &p) == 0x00C80000);
  assert(monthlyPresetPaletteColorAt(5, 2, &p) == 0x00646464);
  assert(monthlyPresetPaletteColorAt(8, 2, &p) == 0x00C80000);

  // Every lit LED on a full 540-LED strip, for every offset, is exactly
  // every 3rd LED and cycles the palette in order with no repeats/skips.
  SchemePalette p3;
  p3.count = 3;
  p3.colors[0] = 1; p3.colors[1] = 2; p3.colors[2] = 3;
  for (uint8_t off = 0; off < MONTHLY_PRESET_LED_STRIDE; off++) {
    int lit = 0;
    for (uint16_t i = 0; i < 540; i++) {
      uint32_t c = monthlyPresetPaletteColorAt(i, off, &p3);
      if (i % MONTHLY_PRESET_LED_STRIDE == off) {
        assert(c == p3.colors[lit % 3]);
        lit++;
      } else {
        assert(c == 0);
      }
    }
    assert(lit == 180);
  }

  // Empty palette -> dark everywhere
  SchemePalette empty;
  empty.count = 0;
  for (uint16_t i = 0; i < 30; i++) assert(monthlyPresetPaletteColorAt(i, 0, &empty) == 0);

  printf("test_scheme_recipe: all passed\n");
  return 0;
}
