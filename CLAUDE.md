# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project state

The sketch is scaffolded and partially functional. `arduino-macropad-led-controller.ino`
wires together the headers below. The verb/noun/menu UI's "Brightness" and
"Presets" branches (rotary knob scroll via the third-party `RotaryEncoder`
library + `<verb>`/`<noun>`/digit entry, OLED rendering, keypad-LED preview)
are implemented as a first coding-pass prototype — see `ui_controller.h` and
`docs/superpowers/specs/2026-08-02-ui-navigation-design.md`. The active
preset and global brightness are persisted across power cycles via flash
(EEPROM emulation, see `settings_storage.h`). Still stubbed: the
"Custom"/pattern-verb branch, and the actual outdoor LED output (type not
yet decided; see "Open design questions" further down) — so a saved
brightness/preset selection isn't yet reflected on real outdoor hardware,
only restored into the UI's own state. There is now a `sketch.yaml`
(Arduino CLI's per-sketch project file — not `arduino-cli.yaml`, which is
that tool's separate *global* config) declaring the board FQBN and expected
libraries, but it's unverified against a live install (its own top comment
explains what to double-check first) and there's still no test setup. None
of this has been compiled against real board/library packages yet, so
verify library API calls (e.g. `Adafruit_MCP23X17`, `Adafruit_SH1106G`
constructor args, `Adafruit_NeoPixel::ColorHSV`, `RotaryEncoder`,
`EEPROM.begin()`/`commit()`) against the installed library versions before
trusting them, and expect the rotary direction (CW vs CCW) to possibly need
swapping once tested against real hardware.

## Project goal

This is the firmware for an **outdoor LED installation** controlled by a
MacroPad, ultimately supporting **12 preset lighting schemes — one per month —
each celebrating something significant in that month**. The MacroPad's 12 keys
map naturally to the 12 monthly presets (press a key to activate that month's
scheme); the rotary encoder and OLED are available for secondary controls
(brightness, speed, manual scheme cycling, status display, etc.) but no
specific UI behavior has been decided yet.

Firmware is written in **Arduino C++, not CircuitPython**, even though most of
Adafruit's official MacroPad documentation/examples default to CircuitPython.

## Hardware architecture

**Adafruit MacroPad RP2040** ([product](https://www.adafruit.com/product/5100), [learn guide](https://learn.adafruit.com/adafruit-macropad-rp2040)) is the controller/brains:
- RP2040 (dual-core Cortex-M0+, ~130MHz, 264KB RAM, 8MB flash)
- 12 mechanical key switches in a 3x4 grid — **not a scanned matrix**; each key is
  wired to its own dedicated GPIO (exposed simply as pins 1–12 in Arduino)
- 12 onboard NeoPixel RGB LEDs, one under each key, all on a single data pin
  (`PIN_NEOPIXEL`, `NUM_NEOPIXEL` = 12) — these are the MacroPad's own indicator
  LEDs, separate from the outdoor LEDs the installation will actually display
- Rotary encoder (20 detents/rev) on `PIN_ROTA`/`PIN_ROTB`, with its own push
  button on `PIN_SWITCH` (this pin also doubles as the BOOT-select button when
  held at power-up/reset — avoid relying on it for anything power-on-critical)
- 128x64 monochrome OLED, SH1106G driver, hardware SPI on `&SPI1`
  (`Adafruit_SH1106G display(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);`)
- 8mm speaker/buzzer on `PIN_SPEAKER` / `PIN_SPEAKER_ENABLE` (Class-D amp + RC filter)
- One **STEMMA QT** connector: a dedicated I2C bus with built-in 10k pull-ups,
  separate from any other I2C on the board

**Adafruit MCP23017 I2C GPIO Expander** ([product](https://www.adafruit.com/product/5346), [learn guide](https://learn.adafruit.com/adafruit-mcp23017-i2c-gpio-expander)), connected to the MacroPad via STEMMA QT, provides the actual outdoor-facing GPIO:
- 16-bit I2C GPIO expander, works at 3V or 5V logic/power
- Default I2C address `0x20`; three address pins (A0/A1/A2) allow up to 8 devices
  on one bus (`0x20`–`0x27`) if the installation grows to need more than 16 pins
- Two STEMMA QT connectors on the breakout allow chaining further STEMMA QT
  devices, but **STEMMA QT only carries I2C (SDA/SCL) + power — no GPIO
  pass-through**
- Each of the 16 GPIO pins can source/sink up to **25mA** — enough to directly
  drive a single LED (with resistor) or a low-current relay/opto module input,
  but higher-current LED strips or larger relay coils will need a
  transistor/MOSFET driver stage rather than a direct connection
- Built-in interrupt support (`setupInterrupts()`, `setupInterruptPin()`, etc.)
  if the design ever needs interrupt-driven inputs rather than polling

**Not yet verified**: the exact Arduino pin/constant names for the MacroPad's
STEMMA QT SDA/SCL lines (likely just `SDA`/`SCL` via the default `Wire`
instance, but confirm against the board variant header before relying on it).

## Arduino toolchain setup

- Board core: Earle Philhower's **arduino-pico** RP2040/RP2350 core, added via
  Arduino IDE Boards Manager URL
  `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
  (this is *not* a separate "Adafruit RP2040" package)
- Board selection: **Tools > Board > Raspberry Pi RP2040 Boards > "Adafruit MacroPad RP2040"**
- There is no Arduino equivalent of CircuitPython's `adafruit_macropad` helper
  library — the NeoPixels, OLED, rotary encoder, and keys are each wired up
  manually with the libraries below rather than through one unified
  MacroPad object

## Expected libraries

- `Adafruit NeoPixel` — onboard MacroPad indicator LEDs (and possibly the
  outdoor LEDs themselves, depending on what LED type is chosen for the
  installation — not yet decided)
- `Adafruit SH110X` — drives the 128x64 OLED (`Adafruit_SH1106G`)
- `RotaryEncoder` (third-party, by mathertel) — decodes the MacroPad's
  rotary encoder; used with `LatchMode::FOUR3`, see `rotary_input.h`
- `Adafruit-MCP23017-Arduino-Library` (GitHub: `adafruit/Adafruit-MCP23017-Arduino-Library`)
  — core class `Adafruit_MCP23XXX`, device subclass `Adafruit_MCP23X17`; API
  mirrors standard Arduino (`pinMode`, `digitalWrite`, `digitalRead` on pins
  0–15, e.g. GPB0 = pin 8)
- `EEPROM` — **not** a separate Library Manager install; bundled with the
  arduino-pico board core itself (mirrors the ESP8266 core's emulated-EEPROM
  API: `begin()`/`read()`/`write()`/`commit()`). Used by `settings_storage.h`
  to persist the active scheme + brightness across power cycles.

## Code architecture & conventions

This project follows the same conventions established in the sibling project
`/home/justin/src/arduino-midi-3-pedal` — refer to that repo's `CLAUDE.md` and
source files as the concrete reference implementation.

**Filenames use underscores, not hyphens** (`led_output.h`, `keypad_input.h`,
`led_presets.h`) — matches every other header in this project and in
`arduino-midi-3-pedal`. Hyphens are harmless to the preprocessor but break
the convention; use underscores for any new file.

**Separation of concerns**: the `.ino` file stays a thin orchestrator
(`setup()`/`loop()` plus top-of-file pin/constant definitions) and each
distinct concern lives in its own header-only `.h` file, `#include`d from the
`.ino`. Each header:
- starts with `#pragma once`
- has a short comment block at the top stating what it does and which
  functions it exposes (its public API)
- contains full function implementations directly in the header (Arduino
  sketches don't compile separate `.cpp` translation units per file — plain
  functions should be `static`/`inline` to avoid multiple-definition errors)

Current header split (each file's own top comment is the source of truth for
its exact responsibility and exposed API — this list is a summary, not a
substitute for reading them):
- `debug.h` — modular serial debug macros (see below)
- `colors.h` — named NeoPixel color constants for the MacroPad's own keypad LEDs
- `gpio_expander.h` — MCP23017 init + pin read/write wrappers for the outdoor-facing GPIO; safe no-ops when `HARDWARE_GPIO_EXPANDER` is undefined
- `led_output.h` — **stub**, drives whatever LED hardware the outdoor installation ends up using (type not yet decided)
- `keypad_input.h` — reads the 12 MacroPad keys, debounced, returns a raw key index (0-11); `keyIsDigit()`/`keyDigitValue()`/`KEY_INDEX_VERB`/`KEY_INDEX_NOUN` give key semantics (pin numbers are a placeholder, not yet verified)
- `rotary_input.h` — rotary encoder (decoded via the third-party `RotaryEncoder` library, `LatchMode::FOUR3`) + switch handling, both implemented; not yet verified against real hardware
- `oled_display.h` — renders the 3-zone UI (menu/message/input) described in the UI/navigation design doc
- `keypad_leds.h` — drives the MacroPad's own 12 onboard NeoPixel LEDs under the keys (distinct from `led_output.h`'s outdoor LEDs); used as a live preview before a selection is confirmed
- `led_presets.h` — the 12 monthly scheme slots/names, active-scheme selection, and keypad-LED preview colors per scheme (`getSchemePreviewColors()` — prototype content, not the final outdoor design); per-scheme *outdoor* render logic is still a stub pending `led_output.h`
- `settings_storage.h` — persists the active scheme + global brightness to flash via the arduino-pico core's built-in EEPROM emulation, restored on boot
- `ui_controller.h` — the verb/noun/menu state machine tying keypad + rotary input + OLED + keypad LEDs + `led_presets.h` + `settings_storage.h` together; implements the flow in `docs/superpowers/specs/2026-08-02-ui-navigation-design.md` (currently: Brightness and Presets branches — Custom/pattern verbs not yet implemented; Brightness is rotary-only, no keypad path yet)

**Modular serial debugging**: mirror `arduino-midi-3-pedal/debug.h` exactly —
a single `DEBUG_SERIAL` macro, defined (or left commented out) in the `.ino`
*before all `#include`s*, gates every debug macro:
```c
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
```
When `DEBUG_SERIAL` is undefined, every call compiles to a no-op with zero
runtime/flash cost — debug instrumentation can stay in the code permanently
rather than being ripped out before "release."

**Optional-hardware feature flags**: if any hardware on this build is
sometimes physically absent (e.g. testing the keypad/rotary logic before the
MCP23017 is wired up), gate it the same way `arduino-midi-3-pedal` gates
`HARDWARE_MIDI`/`HARDWARE_USB` — a `#define HARDWARE_xxx` near the top of the
`.ino`, checked with `#ifdef` in the relevant header, with a same-signature
no-op macro fallback so call sites in the `.ino`/other headers never need
their own `#ifdef`.

**Preprocessor indentation style**: `#ifdef`/`#else`/`#endif` etc. are indented
exactly like a regular `if`/`else` block — the directive matches the
surrounding scope's indentation, and code inside it is indented one further
level (see `arduino-midi-3-pedal/CLAUDE.md`'s "Code style" section for
worked examples).

## Open design questions

These haven't been decided yet — check with the user before assuming an answer:
- What LED type/protocol drives the actual outdoor installation (addressable
  strips like WS2812/NeoPixel vs. simple on/off circuits via MCP23017 GPIO vs.
  relays for higher-power fixtures)?
- What the 12 monthly themes/schemes actually are, and what "significant"
  event each month celebrates
- How presets are selected/stored (key press → immediate switch? rotary knob
  to browse + key/button to confirm? persisted across power cycles?)
- Whether the OLED shows anything beyond a status/preset indicator

## Working in this repo

- There is no established build/upload/lint/test tooling yet. If asked to build
  or upload, confirm `arduino-cli` or Arduino IDE is set up with the board core
  above, and confirm the port before assuming one.
- Favor plain, idiomatic Arduino C++ (`setup()`/`loop()`, standard library
  calls) — no framework beyond the libraries listed above unless the user
  introduces one.
- Follow the file-separation and modular-debug conventions above; when in
  doubt about a pattern, check how `arduino-midi-3-pedal` solved the same kind
  of problem before inventing a new approach.
