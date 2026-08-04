// gpio_expander.h
// Owns the Adafruit MCP23017 I2C GPIO expander connected via STEMMA QT,
// which provides the 16 GPIO pins that actually drive the outdoor
// installation's electronics (relays, MOSFET-driven strips, etc. — see
// led_output.h for what ends up connected to them).
//
// Exposes:
//   initGpioExpander()     — call once in setup()
//   expPinMode(pin, mode)  — pin 0-15 (GPA0-7 = pins 0-7, GPB0-7 = pins 8-15)
//   expWrite(pin, val)
//   expRead(pin)
//
// Gated by HARDWARE_GPIO_EXPANDER, defined in the .ino before all includes.
// When undefined, every call below becomes a safe no-op so keypad/encoder/
// OLED logic can be developed and tested before the MCP23017 is wired up.

#pragma once

#include "debug.h"

#define MCP23017_I2C_ADDR 0x20  // default address, A0/A1/A2 tied low

#ifdef HARDWARE_GPIO_EXPANDER
  #include <Adafruit_MCP23X17.h>

  static Adafruit_MCP23X17 mcp;

  static inline void initGpioExpander() {
    if (!mcp.begin_I2C(MCP23017_I2C_ADDR)) {
      DEBUG_PRINTLN("MCP23017: not found at 0x20");
    } else {
      DEBUG_PRINTLN("MCP23017: initialized");
    }
  }

  static inline void expPinMode(uint8_t pin, uint8_t mode) { mcp.pinMode(pin, mode); }
  static inline void expWrite(uint8_t pin, uint8_t val)    { mcp.digitalWrite(pin, val); }
  static inline int  expRead(uint8_t pin)                  { return mcp.digitalRead(pin); }
#else
  static inline void initGpioExpander()                   { DEBUG_PRINTLN("MCP23017: HARDWARE_GPIO_EXPANDER not defined, skipping"); }
  static inline void expPinMode(uint8_t pin, uint8_t mode) { (void)pin; (void)mode; }
  static inline void expWrite(uint8_t pin, uint8_t val)    { (void)pin; (void)val; }
  static inline int  expRead(uint8_t pin)                  { (void)pin; return 0; }
#endif
