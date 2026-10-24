// ui_controller.h
// The verb/noun/menu interaction state machine that ties the keypad, rotary
// input, OLED, and keypad LEDs together, per
// docs/superpowers/specs/2026-08-02-ui-navigation-design.md.
//
// PROTOTYPE SCOPE — first coding pass, expect to refactor: only the
// "Presets" branch (verb 1) is implemented end to end, reachable either by
// keypad (<verb> 1 <noun> <digits>) or by the OLED menu (rotary scroll +
// press). "Custom" and pattern verbs (2+) are not implemented yet — picking
// "Custom" from the top menu just shows a placeholder hint. "Brightness" is
// intentionally rotary-only, no keypad equivalent — a continuous 0-100
// value fits the rotary knob's turn-to-adjust motion better than digit entry.
//
// Known rough edges, left as-is for now and flagged rather than solved:
//   - Message/input text was condensed to fit the 21-char/line OLED budget
//     (see oled_display.h) rather than the exact wording used while
//     designing this out loud — easy to reword, all of it lives right here.
//   - If you type a noun digit right after turning the rotary knob
//     mid-session, the digit REPLACES that value rather than appending to
//     it (matches "first digit replaces, subsequent digits append" but
//     doesn't special-case an intervening rotary turn). Revisit if it feels
//     wrong in practice.
//   - Confirming always returns to the top-level menu afterward — not
//     explicitly decided, just a reasonable default.
//   - Applying brightness to the *outdoor* installation is still stubbed
//     (debug print only) — no `led_output.h` API exists yet to call. Flash
//     persistence of both the active scheme and brightness, however, is
//     real (see settings_storage.h).
//
// After OLED_IDLE_TIMEOUT_MS (5 min) with no keypad/rotary input, the OLED
// panel powers off (oled_display.h's sleepOledDisplay()) regardless of menu
// state — this doesn't affect anything else (Test Pattern, if running,
// keeps running via its own I2C-driven timer). Any input wakes it and is
// still acted on normally, rather than being swallowed as "just a wake."
//
// The MacroPad works fully with no QT Py attached — Brightness/Presets
// never touch it, and Test Pattern degrades gracefully rather than failing:
// selecting it always enters UI_MENU_TEST_PATTERN and always runs the
// keypad-LED preview with correct sequence/timing (useful for verifying
// Test Pattern on the keypad alone before any outdoor hardware exists),
// independent of whether startTestPatternOnQtpy() actually reached the QT
// Py — a failure there is only logged (DEBUG_PRINT_VAL), never a
// user-facing hint. A persistent "!" icon in the OLED's top-right corner
// (oled_display.h's renderCornerIcon()) shows whenever the QT Py isn't
// found on the I2C bus — checked periodically (QTPY_STATUS_POLL_MS) via a
// non-destructive presence probe (isQtpyConnected()), regardless of menu
// state, so the absence is visible at a glance rather than only after
// trying Test Pattern.
//
// Exposes:
//   initUiController()   — call once in setup(), after keypad/rotary/OLED/
//                           keypad LEDs/schemes/settings storage are all
//                           initialized
//   updateUiController()  — call every loop()

#pragma once

#include <stdio.h>
#include "debug.h"
#include "keypad_input.h"
#include "rotary_input.h"
#include "oled_display.h"
#include "keypad_leds.h"
#include "led_presets.h"
#include "settings_storage.h"
#include <led_test_pattern.h>
#include "qtpy_link.h"

#define UI_HINT_DURATION_MS 2000
#define BRIGHTNESS_STEP_PERCENT 5  // per felt detent; 20 detents/rev * 5% = 100%/rev
#define OLED_IDLE_TIMEOUT_MS (5UL * 60UL * 1000UL)  // 5 minutes of no keypad/rotary input
#define QTPY_STATUS_POLL_MS 1000  // how often to re-probe QT Py presence for the corner icon

enum UiMenuState  { UI_MENU_TOP, UI_MENU_PRESET_LIST, UI_MENU_BRIGHTNESS, UI_MENU_TEST_PATTERN };
enum UiEntryState { UI_ENTRY_IDLE, UI_ENTRY_VERB, UI_ENTRY_NOUN };

static UiMenuState  _uiMenuState  = UI_MENU_TOP;
static UiEntryState _uiEntryState = UI_ENTRY_IDLE;

static int _uiTopHighlight   = 0;      // 0=Brightness, 1=Presets, 2=Custom, 3=Test Pattern
static int _uiVerbCode       = -1;     // -1 = no digit entered yet this verb session
static int _uiPreviewMonth   = 1;      // 1-12, shared by rotary browse + noun digit entry
static bool _uiNounHasDigits = false;  // has at least one digit been typed this noun session
static unsigned long _uiTestPatternStartMs = 0;
static unsigned long _uiLastInputMs = 0;
static bool _uiDisplaySleeping = false;
static bool _uiQtpyConnected = false;        // last-known state, refreshed periodically
static unsigned long _uiQtpyStatusCheckedAt = 0;

// Overwritten from flash in initUiController() (see settings_storage.h) —
// these initializers are just the pre-boot default.
static int _uiConfirmedBrightness = 100;  // 0-100, applied to every preview + (stubbed) outdoor output
static int _uiPendingBrightness   = 100;  // 0-100, live value while UI_MENU_BRIGHTNESS is active

static unsigned long _uiHintUntilMs = 0;
static char _uiHintText[OLED_TEXT_COLS + 1] = "";

static inline void _uiShowHint(const char* text) {
  snprintf(_uiHintText, sizeof(_uiHintText), "%s", text);
  _uiHintUntilMs = millis() + UI_HINT_DURATION_MS;
}

static inline void _uiShowPreviewColors(uint32_t colors[NUM_NEOPIXEL], int brightnessPercent) {
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    colors[i] = scaleColorBrightness(colors[i], brightnessPercent);
  }
  showKeypadLedColors(colors);
}

static inline void _uiUpdatePreview() {
  uint32_t colors[NUM_NEOPIXEL];
  getSchemePreviewColors(_uiPreviewMonth - 1, colors);
  _uiShowPreviewColors(colors, _uiConfirmedBrightness);
}

// Previews the currently-*active* scheme (not whatever's pending in the
// Presets flow) at the in-progress brightness level.
static inline void _uiUpdateBrightnessPreview() {
  uint32_t colors[NUM_NEOPIXEL];
  getSchemePreviewColors(getActiveScheme(), colors);
  _uiShowPreviewColors(colors, _uiPendingBrightness);
}

static inline void _uiRenderMenu() {
  if (_uiMenuState == UI_MENU_TOP) {
    renderMenuTop(_uiTopHighlight);
  } else if (_uiMenuState == UI_MENU_PRESET_LIST) {
    renderMenuPresetList(_uiPreviewMonth, SCHEME_NAMES, NUM_SCHEMES);
  } else if (_uiMenuState == UI_MENU_TEST_PATTERN) {
    renderMenuTestPattern();
  } else {
    renderMenuBrightness(_uiPendingBrightness);
  }
}

static inline void _uiRenderMessage() {
  if (millis() < _uiHintUntilMs) {
    renderMessage(_uiHintText);
  } else if (_uiMenuState == UI_MENU_PRESET_LIST || _uiMenuState == UI_MENU_BRIGHTNESS) {
    renderMessage("Rotary Press: Confirm");
  } else if (_uiMenuState == UI_MENU_TEST_PATTERN) {
    renderMessage("Rotary Press: Exit");
  } else {
    renderMessage("");
  }
}

// Re-asserts the corner icon from the cached connectivity state (no I2C
// traffic here) — call after anything that redraws row 0, since every
// renderMenu*() function redraws that whole row and would otherwise erase
// the icon's cell along with it.
static inline void _uiRenderQtpyIcon() {
  renderCornerIcon(_uiQtpyConnected ? 0 : '!');
}

static inline void _uiRenderInput() {
  char line[OLED_TEXT_COLS + 1];
  if (_uiEntryState == UI_ENTRY_IDLE) {
    renderInputLine("");
  } else if (_uiEntryState == UI_ENTRY_VERB) {
    if (_uiVerbCode < 0) {
      renderInputLine("V: _");
    } else if (_uiVerbCode == 1) {
      snprintf(line, sizeof(line), "V:%d Preset", _uiVerbCode);
      renderInputLine(line);
    } else {
      snprintf(line, sizeof(line), "V:%d ?", _uiVerbCode);
      renderInputLine(line);
    }
  } else {  // UI_ENTRY_NOUN — verb is always 1 (Preset) for now
    if (_uiNounHasDigits) {
      snprintf(line, sizeof(line), "V:1 Preset N:%d", _uiPreviewMonth);
    } else {
      snprintf(line, sizeof(line), "V:1 Preset N:_");
    }
    renderInputLine(line);
  }
}

static TestPatternRenderState _uiTestPatternRenderState;

static inline void _uiShowTestPatternKeypadPreview() {
  TestPatternPhase phase = testPatternPhaseAt(millis() - _uiTestPatternStartMs);
  if (!testPatternPhaseChanged(&_uiTestPatternRenderState, phase)) {
    return;
  }
  uint32_t fullColor = (phase.color == TEST_PATTERN_COLOR_RED)   ? PIXEL_RED
                      : (phase.color == TEST_PATTERN_COLOR_GREEN) ? PIXEL_GREEN
                      :                                             PIXEL_BLUE;
  uint32_t color = scaleColorBrightness(fullColor, LED_TEST_PATTERN_BRIGHTNESS_PERCENT);
  uint32_t colors[NUM_NEOPIXEL];
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    colors[i] = testPatternLedIsLit(i, phase.evenParityLit) ? color : PIXEL_OFF;
  }
  showKeypadLedColors(colors);  // scaled by LED_TEST_PATTERN_BRIGHTNESS_PERCENT, NOT _uiConfirmedBrightness — see the design doc
}

// Shared by _uiConfirm() and the <verb>-key handler in updateUiController()
// — both exit Test Pattern the same way.
static inline void _uiExitTestPattern() {
  stopTestPatternOnQtpy();
  clearKeypadLeds();
  _uiMenuState   = UI_MENU_TOP;
  _uiHintUntilMs = 0;
}

static inline void _uiRenderAll() {
  _uiRenderMenu();
  _uiRenderMessage();
  _uiRenderInput();
  _uiRenderQtpyIcon();
}

// Refreshes the cached QT Py connectivity state at most once every
// QTPY_STATUS_POLL_MS (an I2C presence probe every loop() iteration would
// be needless bus traffic). Only touches the display when the state
// actually changed — _uiRenderAll() already re-asserts the icon on every
// full render, so this is specifically for the "nothing else happens to
// redraw" case (e.g. sitting idle on the Brightness screen while the QT Py
// gets plugged in or unplugged).
static inline void _uiPollQtpyStatus() {
  unsigned long now = millis();
  if (_uiQtpyStatusCheckedAt != 0 && now - _uiQtpyStatusCheckedAt < QTPY_STATUS_POLL_MS) {
    return;
  }
  _uiQtpyStatusCheckedAt = now;

  bool connected = isQtpyConnected();
  if (connected != _uiQtpyConnected) {
    _uiQtpyConnected = connected;
    DEBUG_PRINT_VAL("QT Py connected", _uiQtpyConnected);
    _uiRenderQtpyIcon();
  }
}

static inline void _uiConfirm() {
  if (_uiMenuState == UI_MENU_TEST_PATTERN) {
    _uiExitTestPattern();
  } else if (_uiMenuState == UI_MENU_PRESET_LIST) {
    setActiveScheme(_uiPreviewMonth - 1);
    saveSettings(getActiveScheme(), _uiConfirmedBrightness);
    clearKeypadLeds();
    _uiEntryState  = UI_ENTRY_IDLE;
    _uiMenuState   = UI_MENU_TOP;
    _uiHintUntilMs = 0;
  } else if (_uiMenuState == UI_MENU_BRIGHTNESS) {
    _uiConfirmedBrightness = _uiPendingBrightness;
    // TODO: apply to the outdoor installation — no led_output.h API exists
    // yet to call, see "Persistence" in the design doc. Flash persistence
    // itself is real, below.
    saveSettings(getActiveScheme(), _uiConfirmedBrightness);
    clearKeypadLeds();
    _uiMenuState   = UI_MENU_TOP;
    _uiHintUntilMs = 0;
  } else {
    // UI_MENU_TOP — descend into the highlighted item.
    if (_uiTopHighlight == 0) {
      _uiMenuState = UI_MENU_BRIGHTNESS;
      _uiPendingBrightness = _uiConfirmedBrightness;
      _uiUpdateBrightnessPreview();
    } else if (_uiTopHighlight == 1) {
      _uiMenuState = UI_MENU_PRESET_LIST;
      _uiUpdatePreview();
    } else if (_uiTopHighlight == 2) {
      _uiShowHint("Custom: not built yet");
    } else {
      // Best-effort — the keypad preview below runs regardless of whether
      // the QT Py actually started, so the sequence/timing can be verified
      // on the keypad alone with no outdoor hardware attached at all. QT
      // Py connectivity is already shown ambiently via the corner icon
      // (isQtpyConnected(), polled independently); a failure here only
      // goes to the debug log, not a user-facing hint.
      QtpyLinkResult result = startTestPatternOnQtpy();
      if (result != QTPY_LINK_OK) {
        DEBUG_PRINT_VAL("QT Py start failed, result", result);
      }
      _uiMenuState          = UI_MENU_TEST_PATTERN;
      _uiTestPatternStartMs = millis();
      testPatternResetRenderState(&_uiTestPatternRenderState);
      // A stale UI_ENTRY_VERB from before descending here (e.g. <verb>
      // pressed, then rotated onto Test Pattern without a digit) would
      // let a subsequent digit key fall into _uiHandleDigit()'s verb
      // branch and switch _uiMenuState away without ever stopping the
      // QT Py. Since <verb> always exits Test Pattern rather than
      // re-entering verb entry while it's active, idling this here is
      // sufficient to keep it idle for the whole run.
      _uiEntryState = UI_ENTRY_IDLE;
    }
  }
  _uiRenderAll();
}

static inline void _uiRestartVerbEntry() {
  _uiEntryState    = UI_ENTRY_VERB;
  _uiVerbCode      = -1;
  _uiNounHasDigits = false;
  _uiHintUntilMs   = 0;
  _uiRenderAll();
}

// <verb> normally (re)starts verb entry from scratch ("V: _"). But if it's
// pressed while already sitting at that exact "waiting for a verb digit"
// state, it goes one step further and clears the input line back to idle
// instead — a double-tap-<verb> cancel, rather than restarting into the
// same state it was already in.
static inline void _uiHandleVerbKey() {
  if (_uiEntryState == UI_ENTRY_VERB && _uiVerbCode < 0) {
    _uiEntryState  = UI_ENTRY_IDLE;
    _uiHintUntilMs = 0;
    _uiRenderAll();
  } else {
    _uiRestartVerbEntry();
  }
}

static inline void _uiHandleDigit(uint8_t digit) {
  if (_uiEntryState == UI_ENTRY_VERB) {
    _uiVerbCode = digit;
    if (_uiVerbCode == 1) {
      _uiMenuState = UI_MENU_PRESET_LIST;
      _uiUpdatePreview();
    }
    _uiRenderAll();
  } else if (_uiEntryState == UI_ENTRY_NOUN) {
    int tentative = _uiNounHasDigits ? (_uiPreviewMonth * 10 + digit) : digit;
    if (tentative >= 1 && tentative <= NUM_SCHEMES) {
      _uiPreviewMonth  = tentative;
      _uiNounHasDigits = true;
      _uiUpdatePreview();
      _uiRenderAll();
    }
    // else: ignore the keystroke — it would exceed a valid month number.
  } else {
    _uiShowHint("Press Verb (bottom)");
    _uiRenderAll();
  }
}

static inline void initUiController() {
  int savedScheme, savedBrightness;
  loadSettings(&savedScheme, &savedBrightness);
  setActiveScheme(savedScheme);
  _uiConfirmedBrightness = savedBrightness;
  _uiPendingBrightness   = savedBrightness;
  _uiPreviewMonth        = savedScheme + 1;  // so browsing Presets starts on the saved month, not always January
  _uiLastInputMs         = millis();
  _uiQtpyConnected       = isQtpyConnected();
  _uiQtpyStatusCheckedAt = millis();

  clearKeypadLeds();
  _uiRenderAll();
  DEBUG_PRINTLN("UI controller: initialized");
}

static inline void updateUiController() {
  int keyIndex     = pollKeypad();
  int delta        = pollRotaryDelta();
  bool rotaryPress = rotaryPressed();

  // Wake (and force a redraw) before any of the normal handling below runs,
  // so whatever that handling draws lands on an already-powered-on panel
  // rather than on one still mid-wake. A screen that's been blank for 5+
  // minutes shouldn't eat the input that wakes it — it still acts normally.
  bool hadInput = (keyIndex >= 0) || (delta != 0) || rotaryPress;
  if (hadInput) {
    _uiLastInputMs = millis();
    if (_uiDisplaySleeping) {
      _uiDisplaySleeping = false;
      wakeOledDisplay();
      _uiRenderAll();
    }
  } else if (!_uiDisplaySleeping && millis() - _uiLastInputMs >= OLED_IDLE_TIMEOUT_MS) {
    _uiDisplaySleeping = true;
    sleepOledDisplay();
    DEBUG_PRINTLN("OLED: sleeping (idle timeout)");
  }

  if (keyIndex >= 0) {
    if (keyIndex == KEY_INDEX_VERB) {
      if (_uiMenuState == UI_MENU_TEST_PATTERN) {
        _uiExitTestPattern();
        _uiRenderAll();
      } else {
        _uiHandleVerbKey();
      }
    } else if (keyIndex == KEY_INDEX_NOUN) {
      if (_uiEntryState == UI_ENTRY_VERB && _uiVerbCode == 1) {
        _uiEntryState    = UI_ENTRY_NOUN;
        _uiNounHasDigits = false;
        _uiRenderAll();
      } else if (_uiEntryState == UI_ENTRY_IDLE) {
        _uiShowHint("Press Verb (bottom)");
        _uiRenderAll();
      }
      // else: noun pressed with an unresolved verb — ignored for now.
    } else if (keyIsDigit(keyIndex)) {
      _uiHandleDigit(keyDigitValue(keyIndex));
    }
  }

  if (delta != 0) {
    if (_uiMenuState == UI_MENU_TOP) {
      _uiTopHighlight = (_uiTopHighlight + delta) % 4;
      if (_uiTopHighlight < 0) _uiTopHighlight += 4;
    } else if (_uiMenuState == UI_MENU_PRESET_LIST) {
      int month = ((_uiPreviewMonth - 1 + delta) % NUM_SCHEMES + NUM_SCHEMES) % NUM_SCHEMES + 1;
      _uiPreviewMonth = month;
      _uiUpdatePreview();
    } else if (_uiMenuState == UI_MENU_TEST_PATTERN) {
      // no-op — nothing to adjust while Test Pattern is running
    } else {  // UI_MENU_BRIGHTNESS
      int brightness = _uiPendingBrightness + delta * BRIGHTNESS_STEP_PERCENT;
      if (brightness < 0) brightness = 0;
      if (brightness > 100) brightness = 100;
      _uiPendingBrightness = brightness;
      _uiUpdateBrightnessPreview();
    }
    _uiRenderAll();
  }

  if (rotaryPress) {
    _uiConfirm();
  }

  // A transient hint auto-clears once it expires, even if nothing else
  // triggers a render in the meantime.
  if (_uiHintUntilMs != 0 && millis() >= _uiHintUntilMs) {
    _uiHintUntilMs = 0;
    _uiRenderMessage();
  }

  if (_uiMenuState == UI_MENU_TEST_PATTERN) {
    _uiShowTestPatternKeypadPreview();
  }

  _uiPollQtpyStatus();
}
