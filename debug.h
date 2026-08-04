// debug.h
// Conditional serial debug output — mirrors arduino-midi-3-pedal/debug.h.
// Define DEBUG_SERIAL in arduino-macropad-led-controller.ino BEFORE all
// #includes to enable. When disabled, every macro compiles to nothing with
// zero runtime/flash cost.
//
// Exposes:
//   DEBUG_BEGIN(baud)       — call once in setup()
//   DEBUG_PRINT(x)          — Serial.print()   passthrough
//   DEBUG_PRINTLN(x)        — Serial.println() passthrough
//   DEBUG_PRINT_VAL(lbl, v) — prints "lbl: v"

#pragma once

#ifdef DEBUG_SERIAL
  #define DEBUG_BEGIN(baud)       Serial.begin(baud)
  #define DEBUG_PRINT(x)          Serial.print(x)
  #define DEBUG_PRINTLN(x)        Serial.println(x)
  #define DEBUG_PRINT_VAL(lbl, v) do { Serial.print(lbl); Serial.print(": "); Serial.println(v); } while (0)
#else
  #define DEBUG_BEGIN(baud)       ((void)0)
  #define DEBUG_PRINT(x)          ((void)0)
  #define DEBUG_PRINTLN(x)        ((void)0)
  #define DEBUG_PRINT_VAL(lbl, v) ((void)0)
#endif
