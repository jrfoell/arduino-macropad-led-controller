# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Deployment status

**The installation is live outside (as of 2026-10-31) — do not flash either
board without explicit confirmation from the user first.** Development
continues via commit+push once everything compiles clean; flashing is a
separate, explicitly-requested step now, not automatic.

**Last commit actually flashed to both physical boards: `3928ff6`.**
Update this line every time a flash actually happens, so anyone (including
a fresh session) can tell what's pending. Keep an eye on whether later
commits touch the I2C protocol (`MacropadShared/src/i2c_protocol.h`) — a
protocol change sitting unflashed for a while means the next real flash
needs both boards updated together, not just whichever one's handy, or
they'll misparse each other's messages (see "MacroPad = instructor, QT Py
= executor" below for why the payload shapes have to match exactly).

## Project state

This is now a **monorepo** of two Arduino sketches plus one shared local
library — see "Repo structure" below. The MacroPad sketch
(`arduino-macropad-led-controller/arduino-macropad-led-controller.ino`)
wires together its headers. The verb/noun/menu UI's "Brightness" and
"Presets" branches (rotary knob scroll via the third-party `RotaryEncoder`
library + `<verb>`/`<noun>`/digit entry, OLED rendering, keypad-LED preview)
are implemented as a first coding-pass prototype — see `ui_controller.h` and
`docs/superpowers/specs/2026-08-02-ui-navigation-design.md`. The active
preset and global brightness are persisted across power cycles via flash
(EEPROM emulation, see `settings_storage.h`). Still stubbed: the
"Custom"/pattern-verb branch, and 11 of the 12 monthly presets' real
outdoor content (each month's visual design is an open question — see
"Open design questions" — except **October**).

**MacroPad = instructor, QT Py = executor** (this is the actual protocol
shape now, not just a design principle): the MacroPad is the sole
authority on what any scheme looks like (colors live in `led_presets.h`);
the QT Py has zero compiled-in knowledge of any scheme and just stores +
renders whatever recipe (scheme index, brightness, top/bottom strip
colors) it's told over I2C, persisting it to its own flash independently
so it's also what powers on by default with no MacroPad involved. This
means tuning a color is purely a MacroPad-side edit+reflash — the QT Py
never needs rebuilding for *content* changes, only for actual logic
changes. It's two-way: the MacroPad can also query the QT Py for what
it's currently showing (used when entering the Presets menu, so browsing
starts from the installation's real state rather than a possibly-stale
local copy). See `qtpy_led_driver/outdoor_leds.h`, `scheme_storage.h`,
`i2c_commands.h`, the MacroPad's `qtpy_link.h`, and
`MacropadShared/src/i2c_protocol.h` + `monthly_presets.h` (the latter now
holds only the shared stride/brightness-scaling *math*, no colors — those
live solely in `led_presets.h`).

A fourth menu item, **"Test Pattern"**, is also fully implemented end to
end: it commands the QT Py companion controller over I2C to run a
diagnostic RGB/odd-even cycle across both outdoor strips and the
MacroPad's own keypad LEDs in sync — see
`docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md`.

Each sketch has its own `sketch.yaml` (Arduino CLI's per-sketch project
file — not `arduino-cli.yaml`, which is that tool's separate *global*
config). Both sketches, and `MacropadShared`'s own `g++`-based test suite,
have been **compiled and run for real** against an installed `arduino-cli`
(see "Working in this repo" for the gotchas that took to get there — in
particular, this `arduino-cli` version requires an explicit version on
every `sketch.yaml` profile `libraries:`/`platforms:` entry, and always
use `--profile`, never bare `--fqbn`). Still unverified against real
hardware: the rotary direction (CW vs CCW) may need swapping. The QT Py's
two outdoor-strip GPIO pins (`STRIP_A_PIN`/`STRIP_B_PIN` in
`qtpy_led_driver/outdoor_leds.h`, A0/GPIO29 for the 18m/1st-floor strip,
A1/GPIO28 for the 12m/2nd-floor strip) **are now confirmed working**
end to end through the full QuinLED Diff-Adv Sender-4 → Receiver
Midpoint-1 chain to both real strips.

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

**Outdoor LEDs**: two **WS2812B addressable strips** (BTF-Lighting "ECO,"
30 LEDs/m — 18m/540 LEDs and 12m/360 LEDs). WS2812B needs a precisely-timed
PIO-driven serial data line, which the MacroPad's own GPIO can't supply (its
only expansion point, STEMMA QT, is a fixed-function I2C bus — see below) —
so an **Adafruit QT Py RP2040** companion controller sits on that STEMMA QT
bus, powered over the same cable (its connector's `V+` pin ties directly to
the board's own 3.3V rail), and drives the two strips from its own
PIO-capable GPIO pins on command from the MacroPad over I2C (MacroPad as
host on `Wire`, QT Py as peripheral on `Wire1` — its STEMMA QT connector is
wired to `Wire1`, not the default `Wire`). The strips feed a **QuinLED
Diff-Adv Sender-4**, converting to a differential RS-485 signal over CAT
cable to two daisy-chained **QuinLED Diff-Adv Receiver Midpoint-1** units
near the installation, each locally power-injected from a 24V supply via
its own 24V→5V buck converter. Full signal/power-chain diagram, the
power-budget math behind the "Test Pattern" diagnostic mode, and everything
this paragraph compresses: see
`docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md`.

**Confirmed working end to end**: the QT Py's two outdoor-strip GPIO pins
(A0 -> 18m/1st floor, A1 -> 12m/2nd floor) through the full QuinLED
Diff-Adv Sender-4 → (daisy-chained) Receiver Midpoint-1 → strip chain —
see "Project state".

**Not yet verified**: the MacroPad's own 3.3V regulator headroom for also
powering the QT Py over STEMMA QT (works fine in practice so far, just
never measured).

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

## Repo structure

This is a **monorepo** holding two Arduino sketches plus one shared local
library, as three sibling folders under the repo root (`CLAUDE.md` and
`docs/` stay at the true repo root, since they're project-wide, not
sketch-specific):

- `arduino-macropad-led-controller/` — the MacroPad sketch (moved into its
  own folder rather than living at the repo root, so it explicitly
  satisfies Arduino's one-sketch-one-folder rule rather than relying on
  the repo's own folder name happening to match)
- `qtpy_led_driver/` — the QT Py companion controller's sketch
- `MacropadShared/` — a local Arduino library (`library.properties` +
  `src/`) holding hardware-agnostic logic both sketches need — currently
  the Test Pattern sequencing and the I2C command-byte constants; it also
  has its own `g++`-based test suite (`MacropadShared/test/`), since that
  code has no Arduino dependency

Each sketch's `sketch.yaml` references `MacropadShared` as a local `dir:`
entry (`- dir: ../MacropadShared`) rather than a Library Manager package.
See `docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md`
for why this structure exists.

## Expected libraries

- `Adafruit NeoPixel` — onboard MacroPad indicator LEDs (`keypad_leds.h`)
  and, in the QT Py sketch, the two outdoor WS2812B strips (`outdoor_leds.h`)
- `Adafruit SH110X` — drives the 128x64 OLED (`Adafruit_SH1106G`)
- `RotaryEncoder` (third-party, by mathertel) — decodes the MacroPad's
  rotary encoder; used with `LatchMode::FOUR3`, see `rotary_input.h`
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
- `led_output.h` — **permanent no-op stub**: the MacroPad never drives the outdoor strips directly, not even for implemented months (October included) — that happens entirely on the QT Py, reached over I2C (see `qtpy_link.h`)
- `keypad_input.h` — reads the 12 MacroPad keys, debounced, returns a raw key index (0-11); `keyIsDigit()`/`keyDigitValue()`/`KEY_INDEX_VERB`/`KEY_INDEX_NOUN` give key semantics (pin numbers confirmed against Adafruit's official pinout — see "Hardware architecture")
- `rotary_input.h` — rotary encoder (decoded via the third-party `RotaryEncoder` library, `LatchMode::FOUR3`) + switch handling, both implemented; not yet verified against real hardware
- `oled_display.h` — renders the 3-zone UI (menu/message/input) described in the UI/navigation design doc; top menu is Brightness/Presets/Custom/Test Pattern (4 items)
- `keypad_leds.h` — drives the MacroPad's own 12 onboard NeoPixel LEDs under the keys (distinct from `led_output.h`'s outdoor LEDs); used as a live preview before a selection is confirmed, and for the Test Pattern diagnostic
- `led_presets.h` — the 12 monthly scheme slots/names, active-scheme selection, keypad-LED preview colors per scheme (`getSchemePreviewColors()`), and **the sole source of truth for real outdoor colors** (`getSchemeOutdoorColors()`) — the QT Py has no compiled-in scheme knowledge of its own, this is what gets sent over I2C. October's preview mirrors its real outdoor look exactly (same `OCTOBER_TOP_COLOR`/`OCTOBER_BOTTOM_COLOR` constants feed both) — every other month still returns "off" from `getSchemeOutdoorColors()` and is prototype-only in the preview
- `settings_storage.h` — persists the active scheme + global brightness to flash via the arduino-pico core's built-in EEPROM emulation, restored on boot
- `qtpy_link.h` — MacroPad-side (I2C host) Start/Stop Test Pattern commands, `setActiveSchemeOnQtpy()` (fire-and-forget, sends the full recipe — scheme index, brightness, top/bottom colors from `led_presets.h` — persists on the QT Py; sent on every Preset *and* Brightness confirm), `previewBrightnessOnQtpy()` (fire-and-forget, RAM-only on the QT Py — sent on every Brightness-knob detent while turning it), and `queryActiveSchemeFromQtpy()` (two-way — reads back what the QT Py is actually persisting/showing, used on entering the Presets menu) to the QT Py LED driver, with ACK-based dead-link detection on Start
- `ui_controller.h` — the verb/noun/menu state machine tying keypad + rotary input + OLED + keypad LEDs + `led_presets.h` + `settings_storage.h` + `qtpy_link.h` together; implements the flow in `docs/superpowers/specs/2026-08-02-ui-navigation-design.md` (Brightness, Presets, and Test Pattern branches implemented — Custom/pattern verbs not yet; Brightness is rotary-only, Test Pattern is OLED-menu-only, neither has a keypad path)
- `MacropadShared/src/led_test_pattern.h` — hardware-agnostic Test Pattern color/parity/phase sequencing, shared by both sketches (has a real `g++`-based test suite, no Arduino dependency)
- `MacropadShared/src/i2c_protocol.h` — the Start/Stop/SetScheme/PreviewBrightness/QuerySchemeBrightness/Ping command-byte constants both sketches agree on, plus the poll-interval/link-timeout constants behind the QT Py's host-link status LED. `SET_SCHEME`'s 8-byte payload (scheme, brightness, top RGB, bottom RGB) and `QUERY_SCHEME`'s identically-shaped 8-byte response are the actual "MacroPad = instructor, QT Py = executor" protocol — see "Project state"
- `MacropadShared/src/monthly_presets.h` — only the hardware-agnostic rendering *math* shared by both sketches: `MONTHLY_PRESET_LED_STRIDE` (every 3rd LED) and `monthlyPresetLedIsLit()`/`monthlyPresetScaleColor()`. Deliberately holds no colors or per-month content anymore — those live solely in the MacroPad's `led_presets.h` and get sent over I2C, not compiled into the QT Py
- `qtpy_led_driver/outdoor_leds.h` — drives the two outdoor WS2812B strips from the QT Py's own GPIO for both the Test Pattern sequence and `showActiveSchemeOnOutdoorLeds(topColor, bottomColor, brightnessPercent)` — takes colors directly, no scheme-index dispatch or scheme knowledge of any kind
- `qtpy_led_driver/scheme_storage.h` — persists the QT Py's own active scheme description (index, brightness, top/bottom colors — not just index + brightness) to its own flash (same EEPROM-emulation mechanism as the MacroPad's `settings_storage.h`, but a fully independent flash region) — loaded and re-rendered immediately at the QT Py's own power-up, with no MacroPad involved
- `qtpy_led_driver/i2c_commands.h` — QT Py-side (I2C peripheral on `Wire1`) Start/Stop/SetScheme/PreviewBrightness/QuerySchemeBrightness command handling + ACK/query-response; also tracks last-I2C-contact time for `isHostLinkAlive()` and the current render state for query responses (`setQueryableSchemeState()`)
- `qtpy_led_driver/link_status_led.h` — drives the QT Py's own onboard NeoPixel: green flash on a real received command (Start/Stop, never the PING heartbeat — see `i2c_commands.h`'s `consumeCommandReceivedPending()`), else solid red whenever `isHostLinkAlive()` reports no contact from the MacroPad within `QTPY_LINK_TIMEOUT_MS` (STEMMA QT unplugged, MacroPad off, etc.), else off. `QTPY_CMD_PING` (sent by the MacroPad's presence probe, not a dedicated heartbeat protocol) keeps the link alive without counting as a command
- `qtpy_led_driver/debug.h` — same modular debug macros as the MacroPad's `debug.h`, copied verbatim (each sketch needs its own copy; see `docs/superpowers/plans/2026-10-23-outdoor-led-driver-architecture.md`'s Task 4 for why it isn't shared via `MacropadShared` instead)

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
QT Py companion controller is wired up), gate it the same way `arduino-midi-3-pedal` gates
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
- ~~What LED type/protocol drives the actual outdoor installation~~ —
  **resolved**: addressable WS2812B strips, driven by a QT Py RP2040
  companion controller over I2C and a QuinLED Diff-Adv differential signal
  chain (not anything on the MacroPad's own GPIO or an MCP23017). See
  "Hardware architecture" above and
  `docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md`.
- What the 12 monthly themes/schemes actually are, and what "significant"
  event each month celebrates — **October resolved**: top strip (A1/12m/
  2nd floor) purple, bottom strip (A0/18m/1st floor) orange, both at
  `MONTHLY_PRESET_LED_STRIDE` (every 3rd LED). The other 11 remain open.
- How presets are selected/stored (key press → immediate switch? rotary knob
  to browse + key/button to confirm? persisted across power cycles?)
- Whether the OLED shows anything beyond a status/preset indicator

## Working in this repo

- `arduino-cli` is confirmed working in at least one dev environment, compiling
  both sketches for real. Compile with
  `arduino-cli compile --profile <macropad|qtpy> <sketch-dir>` —
  **always `--profile`, never bare `--fqbn`**: a `sketch.yaml`'s mere
  presence in a sketch directory makes `arduino-cli` auto-engage profile
  resolution regardless of which flags are passed, so bare `--fqbn` fails
  the same way an unversioned `--profile` does. That version requires an
  explicit version on every profile `libraries:`/`platforms:` entry
  (`Name (version)` syntax, checked via `arduino-cli lib list`/`core
  list`) — a bare, unversioned name doesn't resolve even when that exact
  library is already installed. `MacropadShared` has a real test suite
  (`g++ -std=c++11 -I MacropadShared/src MacropadShared/test/test_led_test_pattern.cpp -o /tmp/t && /tmp/t`)
  since it has no Arduino dependency; still confirm `arduino-cli`/Arduino
  IDE is set up before building or uploading on a *different* machine, and
  confirm the port before assuming one.
- Favor plain, idiomatic Arduino C++ (`setup()`/`loop()`, standard library
  calls) — no framework beyond the libraries listed above unless the user
  introduces one.
- Follow the file-separation and modular-debug conventions above; when in
  doubt about a pattern, check how `arduino-midi-3-pedal` solved the same kind
  of problem before inventing a new approach.
