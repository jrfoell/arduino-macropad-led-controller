// arduino-macropad-led-controller.ino
// Adafruit MacroPad RP2040 + a QT Py RP2040 companion controller (STEMMA QT)
// Drives an outdoor LED installation with 12 monthly preset lighting schemes.

// ─── Debug ───────────────────────────────────────────────────────────────────
// Uncomment to enable serial debug output. Must appear before all #includes.
#define DEBUG_SERIAL

#include "debug.h"
#include "colors.h"
#include "led_output.h"
#include "keypad_input.h"
#include "rotary_input.h"
#include "oled_display.h"
#include "keypad_leds.h"
#include "led_presets.h"
#include "settings_storage.h"
#include "qtpy_link.h"
#include "ui_controller.h"

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  DEBUG_BEGIN(115200);

  DEBUG_PRINTLN("Initializing keypad");
  initKeypad();

  DEBUG_PRINTLN("Initializing rotary input");
  initRotary();

  DEBUG_PRINTLN("Initializing OLED");
  initOledDisplay();

  DEBUG_PRINTLN("Initializing keypad LEDs");
  initKeypadLeds();

  DEBUG_PRINTLN("Initializing schemes");
  initSchemes();

  DEBUG_PRINTLN("Initializing settings storage");
  initSettingsStorage();

  DEBUG_PRINTLN("Initializing QT Py link");
  initQtpyLink();

  DEBUG_PRINTLN("Initializing UI controller");
  initUiController();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  updateUiController();
  renderActiveScheme();
}
