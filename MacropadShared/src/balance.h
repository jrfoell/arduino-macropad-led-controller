// balance.h
// Hardware-agnostic math for the global Balance setting: shifts brightness
// between the top strip (A1/12m/2nd floor) and bottom strip (A0/18m/1st
// floor) without ever raising either above the Brightness setting. Used by
// the MacroPad only — it folds Brightness x Balance into one percent per
// strip and sends those (SchemeRecipe.topBrightness/bottomBrightness,
// QTPY_CMD_PREVIEW_BRIGHTNESS), so the QT Py has no notion of balance at
// all. Lives in MacropadShared for its g++ test suite
// (test/test_balance.cpp). See
// docs/superpowers/specs/2026-11-25-balance-design.md.
//
// Balance is 0-100, 50 = even. Linear on each side of center, fully dark
// at the ends (0 = top strip off, 100 = bottom strip off — doubles as a
// maintenance switch for the hard-to-reach top floor):
//   topScale    = balance < 50 ? balance * 2         : 100
//   bottomScale = balance > 50 ? (100 - balance) * 2 : 100
//
// Exposes:
//   BALANCE_DEFAULT_PERCENT — 50 (even)
//   balanceStripScales(balance, outTopScale, outBottomScale) — the 0-100
//                             dimming factor for each strip (balance
//                             clamped to 100)
//   balanceStripPercents(brightness, balance, outTop, outBottom) — each
//                             strip's effective 0-100 percent:
//                             brightness * scale / 100

#pragma once
#include <stdint.h>

#define BALANCE_DEFAULT_PERCENT 50

static inline void balanceStripScales(uint8_t balance, uint8_t* outTopScale, uint8_t* outBottomScale) {
  if (balance > 100) balance = 100;
  *outTopScale    = (balance < 50) ? (uint8_t)(balance * 2) : 100;
  *outBottomScale = (balance > 50) ? (uint8_t)((100 - balance) * 2) : 100;
}

static inline void balanceStripPercents(uint8_t brightness, uint8_t balance, uint8_t* outTop, uint8_t* outBottom) {
  uint8_t topScale, bottomScale;
  balanceStripScales(balance, &topScale, &bottomScale);
  *outTop    = (uint8_t)((uint16_t)brightness * topScale / 100);
  *outBottom = (uint8_t)((uint16_t)brightness * bottomScale / 100);
}
