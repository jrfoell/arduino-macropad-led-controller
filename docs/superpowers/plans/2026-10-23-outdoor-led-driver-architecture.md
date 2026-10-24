# Outdoor LED Driver Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the MCP23017-based outdoor-LED plan with a QT Py RP2040
companion controller driving two WS2812B strips, and ship a "Test Pattern"
diagnostic mode that validates signal/power integrity across the full
18m/12m strip lengths.

**Architecture:** A monorepo with two Arduino sketches, each in its own
sibling folder (the existing MacroPad sketch moved into
`arduino-macropad-led-controller/`, a new QT Py sketch in
`qtpy_led_driver/`) and one shared local library (`MacropadShared/`)
holding hardware-agnostic
sequencing logic and the I2C command-byte constants both sketches need to
agree on. The MacroPad is the I2C host; the QT Py is the I2C peripheral,
generating WS2812 data via two of its own PIO-capable GPIO pins.

**Tech Stack:** Arduino C++ (arduino-pico core), `Adafruit_NeoPixel`,
Arduino `Wire`/`Wire1`, plain `g++` for the one hardware-independent test
target (no Arduino test framework exists in this project).

**Spec:** `docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md`

## Global Constraints

- `LED_TEST_PATTERN_STRIDE = 2` (every-other LED lit).
- `LED_TEST_PATTERN_PHASE_MS = 5000` (5s per color/parity phase).
- Sequence (repeats every 30s): RED-odd → RED-even → GREEN-odd →
  GREEN-even → BLUE-odd → BLUE-even.
- 18m strip = 540 LEDs ("strip A"); 12m strip = 360 LEDs ("strip B").
- Test Pattern renders its single active color channel at
  `LED_TEST_PATTERN_BRIGHTNESS_PERCENT` (50, i.e. 50%) of full (255),
  never the global Brightness setting — raise this constant toward 100
  once a real hardware run confirms the wiring is good.
- Test Pattern never calls `saveSettings()` and never changes the
  confirmed active scheme; exiting returns to the top menu with prior
  state untouched.
- Test Pattern is reachable via the OLED menu only — no keypad verb code
  — and has no preview/confirm two-step: selecting it and pressing the
  rotary starts it immediately.
- MacroPad↔QT Py I2C: MacroPad is host (`Wire`, GPIO20/21, already the
  STEMMA QT pins), QT Py is peripheral on **`Wire1`** (its STEMMA QT
  connector is wired to `Wire1`, GPIO22/23 — **not** the default `Wire`,
  verified against `arduino-pico`'s `adafruit_qtpy` variant header, since
  the default `Wire` there maps to unconnected GPIO24/25). Start command
  gets an ACK byte read back; Stop is fire-and-forget.
- MCP23017 is fully removed: `gpio_expander.h`, `HARDWARE_GPIO_EXPANDER`,
  and the MCP23017 library entry in `sketch.yaml` all go away.
- Board FQBNs (verified live against the installed `rp2040:rp2040` core
  via `arduino-cli board listall`): MacroPad = `rp2040:rp2040:adafruit_macropad2040`
  — **note:** the repo's existing `sketch.yaml` has this wrong
  (`adafruit_macropad`, missing the `2040` suffix — that board id doesn't
  exist in the installed core); fixed as part of Task 1. QT Py =
  `rp2040:rp2040:adafruit_qtpy` (this one was already correct when checked
  against `arduino-pico`'s GitHub source during planning).
- `arduino-cli` (1.5.2-rc.1) is now installed, with the `rp2040:rp2040`
  core and every library both sketches need already present. Every task
  that touches a compilable file ends with a real
  `arduino-cli compile --profile <name> <sketch-dir>` step — see each
  task for the exact command. **Always `--profile`, never bare `--fqbn`:**
  a sketch.yaml's mere presence in a sketch directory makes `arduino-cli`
  auto-engage profile resolution regardless of which flags are passed, so
  bare `--fqbn` fails the same way an unversioned `--profile` does — both
  were tried and failed during Task 1. The actual fix: this `arduino-cli`
  version requires an explicit version on every profile `libraries:` and
  `platforms:` entry (`Name (version)` syntax) — a bare name (this repo's
  original state) doesn't resolve even when that exact library is already
  installed globally. Task 1 pins real installed versions (checked via
  `arduino-cli lib list`/`core list`) in `arduino-macropad-led-controller/sketch.yaml`;
  Task 4 does the same for `qtpy_led_driver/sketch.yaml`.
- `MacropadShared` is referenced from each sketch's `sketch.yaml` as a
  local directory entry — `- dir: ../MacropadShared` in the profile's
  `libraries:` list — confirmed working with a throwaway spike. (The
  `--library <path>` compile flag was the original plan, but it's mutually
  exclusive with `--profile` in this `arduino-cli` version; the `dir:`
  entry is the fix and needs no separate flag.)
- `MacropadShared/src/led_test_pattern.h` has no Arduino dependency, so it
  additionally gets real automated tests run via plain `g++` (Task 2).
- New files follow existing conventions: `#pragma once`, a header comment
  stating purpose + exposed API, `static inline` functions, underscore
  filenames, `DEBUG_SERIAL`/`debug.h` pattern for any new sketch.

## Review Focus

- A reviewer expects a Wire write to a disconnected/unpowered I2C address
  to fail cleanly, not hang or silently report success — `startTestPatternOnQtpy()`
  must return `false` on a non-zero `Wire.endTransmission()` result or a
  missing/wrong ACK byte, and the UI must surface that instead of entering
  Test Pattern as if it worked (Task 6, Task 8).
- A reviewer expects the top-menu's 4 entries to line up identically
  between `ui_controller.h`'s `_uiTopHighlight` indices and
  `oled_display.h`'s `TOP_MENU_ITEMS` array order — an off-by-one here
  silently highlights or confirms the wrong item (Task 7, Task 8).
- A reviewer expects rotary rotation and the `<verb>` key, while Test
  Pattern is running, to do something deliberate (ignore rotation; exit on
  `<verb>`) rather than fall through into Brightness's or the top menu's
  unrelated handling code, which is what happens today if a new
  `UiMenuState` is added without an explicit branch everywhere
  `_uiMenuState` is switched on (Task 8).
- A reviewer expects `millis()` rollover (~49.7 days) not to glitch the
  test-pattern phase/parity calculation, since this is meant to run
  unattended during a field test — `testPatternPhaseAt()` must be checked
  against a value near `ULONG_MAX` (Task 2).
- A reviewer expects leaving Test Pattern to leave zero trace in flash or
  in the confirmed scheme/brightness state — verified by confirming no
  code path in the Test Pattern flow calls `saveSettings()` or
  `setActiveScheme()` (Task 8).

---

## Task 1: Move the MacroPad sketch into its own folder

The repo root currently holds the MacroPad sketch directly (it only
satisfies Arduino's one-sketch-one-folder rule by coincidence, since the
repo's own folder name happens to match `arduino-macropad-led-controller.ino`).
Moving it into an explicit `arduino-macropad-led-controller/` subfolder —
a sibling of `qtpy_led_driver/` and `MacropadShared/` rather than their
implicit parent — makes the monorepo's three-project structure explicit
and symmetric. `CLAUDE.md` and `docs/` stay at the true repo root, since
they're project-wide, not sketch-specific.

**Files:**
- Move: every existing top-level sketch file (`arduino-macropad-led-controller.ino`,
  `sketch.yaml`, `debug.h`, `colors.h`, `gpio_expander.h`, `led_output.h`,
  `keypad_input.h`, `rotary_input.h`, `oled_display.h`, `keypad_leds.h`,
  `led_presets.h`, `settings_storage.h`, `ui_controller.h`) into a new
  `arduino-macropad-led-controller/` subfolder, preserving filenames.

**Interfaces:** none (relocation, plus one content fix — the `sketch.yaml`
FQBN correction below — needed so compile verification works from this
task onward; every later task's `arduino-macropad-led-controller.ino`/
`sketch.yaml`/header references already assume the new location).

- [ ] **Step 1: Move the files**

```bash
mkdir arduino-macropad-led-controller-sketch-tmp  # avoid self-nesting collision
git mv arduino-macropad-led-controller.ino sketch.yaml debug.h colors.h \
  gpio_expander.h led_output.h keypad_input.h rotary_input.h oled_display.h \
  keypad_leds.h led_presets.h settings_storage.h ui_controller.h \
  arduino-macropad-led-controller-sketch-tmp/
git mv arduino-macropad-led-controller-sketch-tmp arduino-macropad-led-controller
```

(The temp-name-then-rename two-step avoids `git mv` trying to move the
files into themselves, since a folder named exactly
`arduino-macropad-led-controller` doesn't exist yet at this point but the
`.ino` file being moved shares that basename.)

- [ ] **Step 2: Verify the move**

Run: `ls arduino-macropad-led-controller/`
Expected: all 13 moved files listed, nothing left at the repo root except
`CLAUDE.md`, `docs/`, `.git`, and other non-sketch project files.

- [ ] **Step 3: Fix the FQBN and pin real versions in `sketch.yaml`**

In `arduino-macropad-led-controller/sketch.yaml`, change
`fqbn: rp2040:rp2040:adafruit_macropad` to
`fqbn: rp2040:rp2040:adafruit_macropad2040` — the id without the `2040`
suffix doesn't exist in the installed `rp2040:rp2040` core (confirmed via
`arduino-cli board listall | grep -i macropad`). Also pin an explicit
version on the `platforms:` entry and every `libraries:` entry (check
real installed versions via `arduino-cli lib list` / `arduino-cli core
list`, format `Name (version)`, e.g. `- platform: rp2040:rp2040 (6.0.0)`
and `- Adafruit NeoPixel (1.15.5)`) — this `arduino-cli` version's profile
resolution fails on a bare, unversioned name even when that exact library
is already installed. Update the file's own top-comment accordingly
(FQBN confirmed; versions pinned and why, not "intentionally omitted").

- [ ] **Step 4: Compile to verify the move and FQBN fix**

Run: `arduino-cli compile --profile macropad arduino-macropad-led-controller/`
Expected: compiles successfully (reports program storage/dynamic memory
usage, no errors) — this is the existing, unmodified sketch, just relocated.
(Always use `--profile`, never bare `--fqbn` — see Global Constraints.)

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor: move the MacroPad sketch into its own folder, fix its FQBN"
```

---

## Task 2: Shared library foundation — test-pattern sequencing + I2C protocol constants

**Files:**
- Create: `MacropadShared/library.properties`
- Create: `MacropadShared/src/led_test_pattern.h`
- Create: `MacropadShared/src/i2c_protocol.h`
- Test: `MacropadShared/test/test_led_test_pattern.cpp`

**Interfaces:**
- Produces (used by Tasks 3, 6, 8):
  - `#define LED_TEST_PATTERN_STRIDE 2`
  - `#define LED_TEST_PATTERN_PHASE_MS 5000`
  - `#define LED_TEST_PATTERN_BRIGHTNESS_PERCENT 50`
  - `enum TestPatternColor { TEST_PATTERN_COLOR_RED, TEST_PATTERN_COLOR_GREEN, TEST_PATTERN_COLOR_BLUE };`
  - `struct TestPatternPhase { TestPatternColor color; bool evenParityLit; };`
  - `TestPatternPhase testPatternPhaseAt(unsigned long elapsedMs)`
  - `bool testPatternLedIsLit(uint16_t ledIndex, bool evenParityLit)`
  - (from `i2c_protocol.h`) `#define QTPY_I2C_ADDRESS 0x55`, `#define QTPY_CMD_STOP 0x00`, `#define QTPY_CMD_START 0x01`, `#define QTPY_ACK_BYTE 0xAC`

- [ ] **Step 1: Write the failing test**

Create `MacropadShared/test/test_led_test_pattern.cpp`:

```cpp
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `g++ -std=c++11 -I MacropadShared/src MacropadShared/test/test_led_test_pattern.cpp -o /tmp/test_led_test_pattern`
Expected: FAIL to compile — `led_test_pattern.h: No such file or directory`

- [ ] **Step 3: Implement `MacropadShared/src/led_test_pattern.h`**

```cpp
#pragma once
#include <stdint.h>

#define LED_TEST_PATTERN_STRIDE              2
#define LED_TEST_PATTERN_PHASE_MS            5000
#define LED_TEST_PATTERN_BRIGHTNESS_PERCENT  50  // starts conservative; raise toward 100 once wiring is confirmed

enum TestPatternColor { TEST_PATTERN_COLOR_RED, TEST_PATTERN_COLOR_GREEN, TEST_PATTERN_COLOR_BLUE };

struct TestPatternPhase {
  TestPatternColor color;
  bool evenParityLit;
};

static inline TestPatternPhase testPatternPhaseAt(unsigned long elapsedMs) {
  unsigned long phaseIndex = (elapsedMs / LED_TEST_PATTERN_PHASE_MS) % 6;
  TestPatternPhase phase;
  phase.color = (TestPatternColor)(phaseIndex / 2);
  phase.evenParityLit = (phaseIndex % 2) == 1;
  return phase;
}

static inline bool testPatternLedIsLit(uint16_t ledIndex, bool evenParityLit) {
  bool isEvenIndex = (ledIndex % LED_TEST_PATTERN_STRIDE) == 0;
  return isEvenIndex == evenParityLit;
}
```

Header comment block: state this is hardware-agnostic sequencing logic
shared by both sketches, and list the exposed API (matching this
project's existing header-comment convention).

- [ ] **Step 4: Create `MacropadShared/src/i2c_protocol.h`**

```cpp
#pragma once

#define QTPY_I2C_ADDRESS 0x55
#define QTPY_CMD_STOP    0x00
#define QTPY_CMD_START   0x01
#define QTPY_ACK_BYTE    0xAC
```

Header comment: these are the only two commands the MacroPad→QT Py link
needs for Test Pattern (see the design doc's "MacroPad ↔ QT Py command
link" section) — not a general preset-rendering protocol.

- [ ] **Step 5: Create `MacropadShared/library.properties`**

```
name=MacropadShared
version=0.1.0
author=Justin Foell
maintainer=Justin Foell
sentence=Hardware-agnostic logic shared between the MacroPad and QT Py sketches.
paragraph=Test-pattern color/parity/phase sequencing and the I2C command-byte constants used between the two controllers.
category=Other
url=
architectures=rp2040
```

- [ ] **Step 6: Run test to verify it passes**

Run: `g++ -std=c++11 -I MacropadShared/src MacropadShared/test/test_led_test_pattern.cpp -o /tmp/test_led_test_pattern && /tmp/test_led_test_pattern`
Expected: prints `All led_test_pattern tests passed`, exit code 0

- [ ] **Step 7: Commit**

```bash
git add MacropadShared/
git commit -m "feat: add shared test-pattern sequencing and I2C protocol constants"
```

---

## Task 3: QT Py outdoor LED rendering

**Files:**
- Create: `qtpy_led_driver/outdoor_leds.h`

**Interfaces:**
- Consumes: `TestPatternPhase`, `testPatternPhaseAt()`, `testPatternLedIsLit()` (Task 2, `MacropadShared/src/led_test_pattern.h`)
- Produces (used by Task 4): `initOutdoorLeds()`, `clearOutdoorLeds()`, `showTestPatternOnOutdoorLeds(unsigned long elapsedMs)`

No standalone compile check here — a header alone isn't a compilable
sketch; this gets its first real `arduino-cli compile` in Task 4, once
`qtpy_led_driver.ino` exists to include it.

- [ ] **Step 1: Implement `qtpy_led_driver/outdoor_leds.h`**

```cpp
#pragma once
#include <Adafruit_NeoPixel.h>
#include "led_test_pattern.h"

// Pin numbers are QT Py "A0"/"A1" pads (GPIO29/GPIO28 per arduino-pico's
// adafruit_qtpy variant header) — NOT YET VERIFIED against real wiring,
// per this project's existing convention; confirm once hardware exists.
#define STRIP_A_PIN       29  // 18m strip
#define STRIP_A_LED_COUNT 540
#define STRIP_B_PIN       28  // 12m strip
#define STRIP_B_LED_COUNT 360

static Adafruit_NeoPixel stripA(STRIP_A_LED_COUNT, STRIP_A_PIN, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel stripB(STRIP_B_LED_COUNT, STRIP_B_PIN, NEO_GRB + NEO_KHZ800);

static inline void initOutdoorLeds();
static inline void clearOutdoorLeds();
static inline void showTestPatternOnOutdoorLeds(unsigned long elapsedMs);
```

Implementation approach for the three functions:
- `initOutdoorLeds()`: `begin()` + `show()` on both strips.
- `clearOutdoorLeds()`: `clear()` + `show()` on both strips.
- `showTestPatternOnOutdoorLeds(elapsedMs)`: call `testPatternPhaseAt(elapsedMs)`;
  track the last-rendered `TestPatternPhase` in a static and return early
  if unchanged (avoids re-pushing ~540/360 pixels every `loop()` iteration
  when nothing changed); otherwise map `phase.color` to a single-channel
  color scaled by `LED_TEST_PATTERN_BRIGHTNESS_PERCENT` — compute
  `uint8_t level = (uint16_t)255 * LED_TEST_PATTERN_BRIGHTNESS_PERCENT / 100;`
  once, then `Color(level,0,0)` / `Color(0,level,0)` / `Color(0,0,level)`
  for RED/GREEN/BLUE (at the default 50%, `level` = 127 — half the design
  doc's full-brightness power-budget numbers, so 9W/36% and 6W/24% of the
  25W fuse budget for the 18m/12m strips) — then for each strip,
  `setPixelColor(i, testPatternLedIsLit(i, phase.evenParityLit) ? color : 0)`
  for every index, and `show()`.

- [ ] **Step 2: Commit**

```bash
git add qtpy_led_driver/outdoor_leds.h
git commit -m "feat: add QT Py outdoor NeoPixel test-pattern rendering"
```

---

## Task 4: QT Py I2C command handling + sketch wiring

**Files:**
- Create: `qtpy_led_driver/i2c_commands.h`
- Create: `qtpy_led_driver/debug.h` (copy of `arduino-macropad-led-controller/debug.h` — see note below)
- Create: `qtpy_led_driver/qtpy_led_driver.ino`
- Create: `qtpy_led_driver/sketch.yaml`

**Interfaces:**
- Consumes: `QTPY_I2C_ADDRESS`, `QTPY_CMD_START`, `QTPY_CMD_STOP`, `QTPY_ACK_BYTE` (Task 2); `initOutdoorLeds()`, `clearOutdoorLeds()`, `showTestPatternOnOutdoorLeds()` (Task 3)
- Produces: `initI2cCommands()`, `isTestPatternRunning()` — consumed only within this sketch's own `.ino`.

**Note on `debug.h`:** this project's `debug.h` is a tiny, board-agnostic
macro file (no Arduino API beyond `Serial`, which exists on both boards).
Rather than pull it into `MacropadShared` (out of scope per the design
doc, which scopes that library to the test-pattern/I2C-protocol pieces
only) or rewire the MacroPad sketch's existing working include, copy it
verbatim into `qtpy_led_driver/`.

- [ ] **Step 1: Copy `debug.h` into the new sketch**

```bash
cp arduino-macropad-led-controller/debug.h qtpy_led_driver/debug.h
```

- [ ] **Step 2: Implement `qtpy_led_driver/i2c_commands.h`**

```cpp
#pragma once
#include <Wire.h>
#include "debug.h"
#include "outdoor_leds.h"
#include "i2c_protocol.h"  // from MacropadShared

static inline void initI2cCommands();
static inline bool isTestPatternRunning();
```

Implementation approach:
- A `static volatile bool _testPatternRunning = false;`
- `onReceive` handler: read the first byte; if no bytes available, return;
  set `_testPatternRunning = (cmd == QTPY_CMD_START)`; if the command was
  `QTPY_CMD_STOP`, call `clearOutdoorLeds()` immediately so the strips go
  dark the instant Stop arrives rather than waiting for the next `loop()`
  pass; drain any extra bytes Wire buffered.
- `onRequest` handler: `Wire.write(QTPY_ACK_BYTE)` unconditionally — per
  the design doc, Stop is fire-and-forget so the host only ever calls
  `requestFrom()` after Start, making unconditional ACK correct.
- **Use `Wire1`, not `Wire`**: `Wire1.begin(QTPY_I2C_ADDRESS)`,
  `Wire1.onReceive(...)`, `Wire1.onRequest(...)` — see Global Constraints
  for why (the QT Py's STEMMA QT connector is wired to `Wire1`, not the
  default `Wire`).

- [ ] **Step 3: Implement `qtpy_led_driver/qtpy_led_driver.ino`**

```cpp
// qtpy_led_driver.ino
// Adafruit QT Py RP2040 — I2C peripheral (Wire1/STEMMA QT) that drives the
// two outdoor WS2812B strips on command from the MacroPad controller.
// See docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md

#define DEBUG_SERIAL

#include "debug.h"
#include "outdoor_leds.h"
#include "i2c_commands.h"

static unsigned long _testPatternStartMs = 0;
static bool _wasRunning = false;

void setup() {
  DEBUG_BEGIN(115200);
  initOutdoorLeds();
  initI2cCommands();
  DEBUG_PRINTLN("QT Py LED driver: initialized");
}

void loop() {
  bool running = isTestPatternRunning();
  if (running && !_wasRunning) {
    _testPatternStartMs = millis();
  }
  if (running) {
    showTestPatternOnOutdoorLeds(millis() - _testPatternStartMs);
  }
  _wasRunning = running;
}
```

- [ ] **Step 4: Create `qtpy_led_driver/sketch.yaml`**

```yaml
# sketch.yaml
# Arduino CLI Sketch Project File for the QT Py RP2040 companion LED driver.
# See arduino-macropad-led-controller/sketch.yaml for the general pattern
# this follows, and
# docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md
# for why this sketch exists.
#
# `fqbn` confirmed via `arduino-cli board listall`. Library/platform
# versions are pinned to what's actually installed (checked via
# `arduino-cli lib list` / `arduino-cli core list`) — this `arduino-cli`
# version's profile resolution requires an explicit version on every
# entry, confirmed while building arduino-macropad-led-controller/sketch.yaml
# in Task 1. `MacropadShared` is referenced as a local `dir:` entry since
# it isn't a Library Manager package.
#
# STILL NOT YET VERIFIED — no real hardware attached while this was written:
#   - STRIP_A_PIN/STRIP_B_PIN in outdoor_leds.h (GPIO29/28, the board's
#     "A0"/"A1" pads) are placeholders — confirm against real wiring.

profiles:
  qtpy:
    fqbn: rp2040:rp2040:adafruit_qtpy
    platforms:
      - platform: rp2040:rp2040 (6.0.0)
        platform_index_url: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
    libraries:
      - Adafruit NeoPixel (1.15.5)
      - dir: ../MacropadShared

default_profile: qtpy
```

(Pin whatever version `arduino-cli lib list`/`core list` actually show at
implementation time — 1.15.5/6.0.0 are what's installed as of this plan's
writing, confirmed in Task 1.)

- [ ] **Step 5: Verify by inspection**

Run: `grep -rn "Wire\." qtpy_led_driver/i2c_commands.h`
Expected: every call uses `Wire1`, never bare `Wire`.

- [ ] **Step 6: Compile the QT Py sketch**

Run: `arduino-cli compile --profile qtpy qtpy_led_driver/`
Expected: compiles successfully — this is the first point the QT Py
sketch (Tasks 3+4 together) and its use of `MacropadShared` can actually
be verified end to end. (Always `--profile`, never bare `--fqbn`/`--library`
— see Global Constraints.)

- [ ] **Step 7: Commit**

```bash
git add qtpy_led_driver/
git commit -m "feat: add QT Py I2C command handling and sketch wiring"
```

---

## Task 5: Remove the MCP23017 from the MacroPad sketch

**Files:**
- Delete: `arduino-macropad-led-controller/gpio_expander.h`
- Modify: `arduino-macropad-led-controller/arduino-macropad-led-controller.ino`
- Modify: `arduino-macropad-led-controller/sketch.yaml`

**Interfaces:** none (pure removal; nothing else in the codebase after
this task references `gpio_expander.h`, `HARDWARE_GPIO_EXPANDER`,
`initGpioExpander()`, `expPinMode()`, `expWrite()`, or `expRead()`).

- [ ] **Step 1: Delete `gpio_expander.h`**

```bash
git rm arduino-macropad-led-controller/gpio_expander.h
```

- [ ] **Step 2: Remove the MCP23017 references from `arduino-macropad-led-controller.ino`**

Remove the `HARDWARE_GPIO_EXPANDER` feature-flag comment+define, the
`#include "gpio_expander.h"` line, and the `initGpioExpander()` call (and
its preceding `DEBUG_PRINTLN`) from `setup()` in
`arduino-macropad-led-controller/arduino-macropad-led-controller.ino`.

- [ ] **Step 3: Remove the MCP23017 library entry from `sketch.yaml`**

Remove the `- Adafruit MCP23017 Arduino Library` line and its preceding
comment from the `libraries:` list in
`arduino-macropad-led-controller/sketch.yaml`.

- [ ] **Step 4: Verify by inspection**

Run: `grep -rn -i "mcp23017\|gpio_expander\|HARDWARE_GPIO_EXPANDER" --include=*.ino --include=*.h --include=*.yaml .`
Expected: no output.

- [ ] **Step 5: Compile to verify the removal**

Run: `arduino-cli compile --profile macropad arduino-macropad-led-controller/`
Expected: compiles successfully with no reference to the MCP23017 library.
(Also remove the now-unused `Adafruit MCP23017 Arduino Library (version)`
line from `sketch.yaml`'s `libraries:` list as part of Step 3 above —
the profile would otherwise still download a library nothing uses.)

- [ ] **Step 6: Commit**

```bash
git add -u
git commit -m "refactor: remove MCP23017 GPIO expander, superseded by the QT Py driver"
```

---

## Task 6: MacroPad-side I2C link to the QT Py

**Files:**
- Create: `arduino-macropad-led-controller/qtpy_link.h`

**Interfaces:**
- Consumes: `QTPY_I2C_ADDRESS`, `QTPY_CMD_START`, `QTPY_CMD_STOP`, `QTPY_ACK_BYTE` (Task 2)
- Produces (used by Task 8): `initQtpyLink()`, `bool startTestPatternOnQtpy()`, `stopTestPatternOnQtpy()`

No compile check here — nothing includes `qtpy_link.h` yet (that happens
in Task 8, which is also where it gets compiled for real). See the "I2C
link failure" Review Focus item above, which this task's return-value
contract exists to satisfy.

- [ ] **Step 1: Implement `qtpy_link.h`**

```cpp
#pragma once
#include <Wire.h>
#include "debug.h"
#include "i2c_protocol.h"  // from MacropadShared

static inline void initQtpyLink() {
  Wire.begin();
  DEBUG_PRINTLN("QT Py link: initialized");
}

static inline bool startTestPatternOnQtpy();
static inline void stopTestPatternOnQtpy();
```

Implementation approach for `startTestPatternOnQtpy()`: write
`QTPY_CMD_START` via `Wire.beginTransmission`/`write`/`endTransmission`;
return `false` immediately if `endTransmission()` doesn't return `0`;
otherwise `Wire.requestFrom(QTPY_I2C_ADDRESS, 1)` and return `false` if
fewer than 1 byte is available; otherwise return whether the byte read
equals `QTPY_ACK_BYTE`. This exact true/false contract is what Task 8's UI
error-hint path depends on.

Implementation approach for `stopTestPatternOnQtpy()`: write
`QTPY_CMD_STOP` the same way, no response read (fire-and-forget per the
design doc).

- [ ] **Step 2: Commit**

```bash
git add arduino-macropad-led-controller/qtpy_link.h
git commit -m "feat: add MacroPad-side I2C link to the QT Py LED driver"
```

---

## Task 7: OLED — 4th top-level menu item

**Files:**
- Modify: `arduino-macropad-led-controller/oled_display.h`

**Interfaces:**
- Produces (used by Task 8): `renderMenuTop(highlightIndex)` now expects
  `highlightIndex` 0-3 (0=Brightness, 1=Presets, 2=Custom, 3=Test Pattern);
  new `renderMenuTestPattern()`.

- [ ] **Step 1: Update `renderMenuTop` to 4 items**

Change `TOP_MENU_ITEMS` from a 3-entry to a 4-entry array ending in
`"Test Pattern"`, and change the loop's `if (row < 3)` to `if (row < 4)`.
Update the header comment's `renderMenuTop(highlightIndex)` line to list
all 4 indices.

- [ ] **Step 2: Add `renderMenuTestPattern()`**

```cpp
static inline void renderMenuTestPattern() {
  _drawRow(0, "Test Pattern");
  _drawRow(1, "Running...");
  for (int row = 2; row < OLED_MENU_ROWS; row++) _drawRow(row, "");
  display.display();
}
```

Add it to the header comment's exposed-API list.

- [ ] **Step 3: Compile to verify**

Run: `arduino-cli compile --profile macropad arduino-macropad-led-controller/`
Expected: compiles successfully (nothing calls `renderMenuTestPattern()`
yet — that's Task 8 — but the file must still compile on its own merits).

- [ ] **Step 4: Commit**

```bash
git add arduino-macropad-led-controller/oled_display.h
git commit -m "feat: add Test Pattern as a 4th top-level OLED menu item"
```

---

## Task 8: `ui_controller.h` — Test Pattern menu state, start/stop, keypad sync

**Files:**
- Modify: `arduino-macropad-led-controller/ui_controller.h`
- Modify: `arduino-macropad-led-controller/arduino-macropad-led-controller.ino`

**Interfaces:**
- Consumes: `TestPatternPhase`, `testPatternPhaseAt()`, `testPatternLedIsLit()` (Task 2); `initQtpyLink()`, `startTestPatternOnQtpy()`, `stopTestPatternOnQtpy()` (Task 6); `renderMenuTestPattern()` (Task 7); existing `PIXEL_RED`/`PIXEL_GREEN`/`PIXEL_BLUE` (`colors.h`), `showKeypadLedColors()`/`clearKeypadLeds()` (`keypad_leds.h`)

This is the integration task; see the Review Focus items above (menu-index
alignment, explicit branches for the new state, no `saveSettings()` call)
— each is addressed by a specific step below, plus a real compile check
(Steps 10-11) — full OLED/I2C/hardware behavior still can't be verified
without the physical board, QT Py, and the signal chain in place.

- [ ] **Step 1: Add includes and the new menu state**

Add `#include <led_test_pattern.h>` and `#include "qtpy_link.h"`. Add
`UI_MENU_TEST_PATTERN` to the `UiMenuState` enum. Add
`static unsigned long _uiTestPatternStartMs = 0;`.

- [ ] **Step 2: Widen the top-menu cycle from 3 to 4 items**

In `updateUiController()`'s rotary-delta handling, change
`_uiTopHighlight = (_uiTopHighlight + delta) % 3;` (and the matching
`+= 3` underflow fix) to `% 4` / `+= 4`. Update the `_uiTopHighlight`
declaration's comment to `0=Brightness, 1=Presets, 2=Custom, 3=Test Pattern`.

- [ ] **Step 3: Render the new state**

In `_uiRenderMenu()`, add `else if (_uiMenuState == UI_MENU_TEST_PATTERN) renderMenuTestPattern();`.
In `_uiRenderMessage()`'s condition that currently shows "Rotary Press:
Confirm" for `UI_MENU_PRESET_LIST || UI_MENU_BRIGHTNESS`, add
`UI_MENU_TEST_PATTERN` as a third case showing `"Verb: Exit"` instead (it
has no confirm step to prompt for — only an exit).

- [ ] **Step 4: Start Test Pattern on confirm**

In `_uiConfirm()`'s `UI_MENU_TOP` branch, change the existing
`if (_uiTopHighlight == 0) {...} else if (_uiTopHighlight == 1) {...} else { _uiShowHint("Custom: not built yet"); }`
to add an explicit `else if (_uiTopHighlight == 2)` for the existing
"Custom: not built yet" hint, and a new final
`else if (_uiTopHighlight == 3)` that: calls `startTestPatternOnQtpy()`;
on `true`, sets `_uiMenuState = UI_MENU_TEST_PATTERN` and
`_uiTestPatternStartMs = millis()`; on `false`, calls
`_uiShowHint("QT Py: no ACK")` and leaves `_uiMenuState` at `UI_MENU_TOP`.

- [ ] **Step 5: Exit Test Pattern on confirm (rotary press) or `<verb>`**

In `_uiConfirm()`, add a new branch checked **before** the existing
`UI_MENU_PRESET_LIST`/`UI_MENU_BRIGHTNESS`/else-TOP chain:
`if (_uiMenuState == UI_MENU_TEST_PATTERN) { stopTestPatternOnQtpy(); clearKeypadLeds(); _uiMenuState = UI_MENU_TOP; _uiHintUntilMs = 0; }`
(then `return`/`else if` into the existing chain unchanged below it).

In `updateUiController()`'s keypad handling, before the existing
`if (keyIndex == KEY_INDEX_VERB) { _uiHandleVerbKey(); }` call, add a
check: if `_uiMenuState == UI_MENU_TEST_PATTERN`, run the same
stop/clear/return-to-top logic as the confirm-exit above instead of
calling `_uiHandleVerbKey()`.

- [ ] **Step 6: Ignore rotary rotation during Test Pattern**

In `updateUiController()`'s rotary-delta handling, the current
`if (UI_MENU_TOP) {...} else if (UI_MENU_PRESET_LIST) {...} else { // UI_MENU_BRIGHTNESS }`
chain implicitly treats anything else as Brightness. Add an explicit
`else if (_uiMenuState == UI_MENU_TEST_PATTERN) { /* no-op — nothing to adjust */ }`
branch before the final `else`, so Test Pattern doesn't fall into
Brightness's adjustment logic.

- [ ] **Step 7: Drive the keypad LEDs in sync, every loop, while running**

Add a static helper:

```cpp
static inline void _uiShowTestPatternKeypadPreview() {
  TestPatternPhase phase = testPatternPhaseAt(millis() - _uiTestPatternStartMs);
  uint32_t fullColor = (phase.color == TEST_PATTERN_COLOR_RED)   ? PIXEL_RED
                      : (phase.color == TEST_PATTERN_COLOR_GREEN) ? PIXEL_GREEN
                      :                                             PIXEL_BLUE;
  uint32_t color = scaleColorBrightness(fullColor, LED_TEST_PATTERN_BRIGHTNESS_PERCENT);
  uint32_t colors[NUM_NEOPIXEL];
  for (uint8_t i = 0; i < NUM_NEOPIXEL; i++) {
    colors[i] = testPatternLedIsLit(i, phase.evenParityLit) ? color : PIXEL_OFF;
  }
  showKeypadLedColors(colors);  // scaled by LED_TEST_PATTERN_BRIGHTNESS_PERCENT, NOT _uiConfirmedBrightness — see Global Constraints
}
```

(`scaleColorBrightness()` is the existing helper already in `colors.h`,
already included by `ui_controller.h` — no new helper needed.)

At the end of `updateUiController()` (so it updates every loop pass, not
just on a keypress/rotary event), add:
`if (_uiMenuState == UI_MENU_TEST_PATTERN) { _uiShowTestPatternKeypadPreview(); }`

(Simpler than the strip renderer's change-detection optimization in Task
2 — 12 keypad LEDs is cheap enough to re-push unconditionally every loop.)

- [ ] **Step 8: Wire `initQtpyLink()` into `setup()`**

In `arduino-macropad-led-controller/arduino-macropad-led-controller.ino`,
add `#include "qtpy_link.h"` and an `initQtpyLink();` call in `setup()`
(alongside the existing `DEBUG_PRINTLN` calls, matching the file's
existing style).

- [ ] **Step 9: Verify by inspection**

Confirm every `if/else if` chain in `ui_controller.h` that switches on
`_uiMenuState` (`_uiRenderMenu`, `_uiRenderMessage`, `_uiConfirm`, the
rotary-delta handler) has an explicit `UI_MENU_TEST_PATTERN` arm — no
implicit `else` should be reached when `_uiMenuState == UI_MENU_TEST_PATTERN`.
Confirm `oled_display.h`'s `TOP_MENU_ITEMS` order (Brightness, Presets,
Custom, Test Pattern) matches `_uiTopHighlight`'s 0/1/2/3 mapping in
`_uiConfirm()` exactly.
Run: `grep -n "saveSettings\|setActiveScheme" arduino-macropad-led-controller/ui_controller.h`
and confirm neither call appears inside any Test-Pattern-related code
added in this task.

- [ ] **Step 10: Add `MacropadShared` to this sketch's `sketch.yaml`**

This is the first point `arduino-macropad-led-controller`'s own code
(`ui_controller.h`, via `led_test_pattern.h` and `qtpy_link.h`'s
`i2c_protocol.h`) uses `MacropadShared`. Add
`- dir: ../MacropadShared` to the `libraries:` list in
`arduino-macropad-led-controller/sketch.yaml` (same pattern as Task 4's
`qtpy_led_driver/sketch.yaml`).

- [ ] **Step 11: Compile the full MacroPad sketch**

Run: `arduino-cli compile --profile macropad arduino-macropad-led-controller/`
Expected: compiles successfully — this is the first point the complete
Test Pattern feature (shared library, QT Py link, OLED, and the
`ui_controller.h` integration) compiles together as the real MacroPad
sketch. (Always `--profile`, never bare `--fqbn`/`--library` — see Global
Constraints.)

- [ ] **Step 12: Commit**

```bash
git add arduino-macropad-led-controller/ui_controller.h arduino-macropad-led-controller/arduino-macropad-led-controller.ino arduino-macropad-led-controller/sketch.yaml
git commit -m "feat: add Test Pattern menu — start/stop via QT Py, synced keypad preview"
```

---

## Task 9: Documentation — update CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:** none (documentation only).

- [ ] **Step 1: Resolve the LED-type open design question**

In "Open design questions," remove or mark resolved the "What LED
type/protocol drives the actual outdoor installation" bullet, replacing
it with a one-line pointer to the new architecture (addressable WS2812B
via a QT Py companion controller + QuinLED Diff-Adv differential signal
chain — see the design doc).

- [ ] **Step 2: Update "Hardware architecture"**

Remove the MCP23017 as the outdoor-GPIO path; add a short section (or
amend the existing MCP23017 entry) describing: the QT Py RP2040 companion
controller on the MacroPad's STEMMA QT bus (powered over that same cable),
the two WS2812B strips (540/360 LEDs), and the QuinLED Diff-Adv
Sender-4 → (daisy-chained) Receiver Midpoint-1 ×2 signal chain, with a
pointer to the design doc for the full diagram and power-budget math.

- [ ] **Step 3: Update "Current header split"**

Remove the `gpio_expander.h` entry. Add entries for
`qtpy_led_driver/outdoor_leds.h`, `qtpy_led_driver/i2c_commands.h`,
`qtpy_led_driver/debug.h`, `arduino-macropad-led-controller/qtpy_link.h`,
and `MacropadShared/src/led_test_pattern.h` / `i2c_protocol.h`, each with
the one-line responsibility already given in their own header comments.

- [ ] **Step 4: Update "Expected libraries"**

Remove the `Adafruit-MCP23017-Arduino-Library` entry.

- [ ] **Step 5: Add a "Repo structure" note**

State plainly that this is now a monorepo with three sibling folders
under the repo root: `arduino-macropad-led-controller/` (the MacroPad
sketch, moved into its own folder rather than living at the repo root),
`qtpy_led_driver/` (the QT Py's sketch), and `MacropadShared/` (a local
Arduino library both depend on) — with a pointer to the design doc for why.

- [ ] **Step 6: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md for the QT Py outdoor LED architecture"
```
