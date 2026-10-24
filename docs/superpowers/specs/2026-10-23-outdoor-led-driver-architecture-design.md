# Outdoor LED Driver Architecture — MacroPad LED Controller

**Status:** DRAFT — builder-checked, physical layout confirmed with you.
Ready for your final review before moving to an implementation plan.

## Intent and why

The project's original hardware plan routed the outdoor installation's LEDs
through the MCP23017 I2C GPIO expander (see `gpio_expander.h`, and
CLAUDE.md's now-resolved "Open design questions"). That plan assumed either
simple on/off circuits or relays. This design session picked the actual LED
hardware — addressable WS2812B strips — and discovered that choice is
incompatible with the MCP23017 and, as it turned out, with the MacroPad
RP2040's own GPIO entirely. This document captures the resulting hardware
architecture (a second microcontroller, a long-distance differential signal
chain, and a monorepo restructure) and the first concrete feature built on
top of it: a **Test Pattern** diagnostic mode that validates signal and
power integrity across the longest strip run before any mid-strip power
injection is wired in.

Everything below was worked out collaboratively; where a conclusion rests on
an external fact (a datasheet number, a pinout, a schematic net), the
specific source is cited so it can be re-verified against real hardware
later, per this project's existing "verify before trusting" convention.

## Goals

- Drive two WS2812B strips (18m/540 LEDs and 12m/360 LEDs) from this
  project's firmware, replacing the MCP23017-based plan.
- Build a **Test Pattern** mode that proves signal and power reach the full
  length of each strip, safely within the receiver hardware's stock fuse
  rating, before any mid-strip power injection exists.
- Restructure the repo into a monorepo holding both microcontrollers'
  firmware plus a shared library, without breaking either sketch's
  buildability.
- Keep the new Test Pattern menu item consistent with this project's
  existing OLED-menu conventions (`ui_controller.h`,
  `2026-08-02-ui-navigation-design.md`) where that convention still fits a
  diagnostic (non-production) feature.

## Non-goals (explicitly out of scope here)

- The 12 monthly presets' actual visual content/rendering on the real
  strips — still an open question (CLAUDE.md), untouched by this doc.
- The "Custom" pattern-builder verb — still a stub.
- Final power-injection design (how many buck converters, where, wire
  gauge) for the completed installation — this doc only covers the
  **test-mode** power budget, which deliberately avoids needing injection.
- Any rework of the MacroPad RP2040 board itself (bodge wires to the RP2040
  package, etc.) — considered and rejected; see "Rejected alternatives."

## Anti-goals

- A test mode that can silently exceed the receiver's fuse rating — all
  brightness/density choices below are deliberately conservative against
  that limit.
- Over-building the MacroPad↔QT Py command protocol ahead of the preset
  rendering design that hasn't happened yet — the protocol below covers
  only what Test Pattern needs.
- Losing this project's dual-keypad/OLED-menu-accessibility principle for
  features where it actually makes sense — Test Pattern is the deliberate,
  documented exception (see "Test Pattern UI").

## Hardware architecture (decided)

### Why the MCP23017 can't drive WS2812B, even without animation

This isn't a frame-rate limitation. WS2812B has no register-style interface
— each LED is a shift-register stage, and the *only* way to set its color,
even once, is to stream a serial bitstream down its data line at ~800kHz
with sub-microsecond, tightly-toleranced pulse widths (~0.4µs/~0.8µs
high-time distinguishing a "0" bit from a "1" bit, ±~150ns tolerance). That
requires dedicated timing hardware (the RP2040's PIO blocks are built for
exactly this) or a cycle-counted bit-bang loop. The MCP23017 only exposes
slow, transaction-based GPIO over I2C (every pin change costs a full
Start+Address+Register-write+Stop, and I2C itself tops out around
400kHz–1MHz) — there's no way to shape that into the needed bitstream. A
simple on/off LED or relay is fine with "slow, occasional transitions";
WS2812B always needs that one precise programming step, regardless of
whether the content changes after that. **Conclusion: the MCP23017 is
removed from the BOM.**

### The MacroPad RP2040 has no spare GPIO

Verified against the actual board sources (not just the learn-guide
overview), to avoid repeating this project's past habit of untrusted
pin-number placeholders:

- `arduino-pico`'s board variant header
  (`variants/adafruit_macropad2040/pins_arduino.h` in
  `earlephilhower/arduino-pico`) confirms `PIN_WIRE0_SDA = GPIO20` and
  `PIN_WIRE0_SCL = GPIO21` — these are the STEMMA QT I2C pins, already
  carrying I2C traffic (previously to the MCP23017).
- Adafruit's own official pinout diagram (`Adafruit MacroPad RP2040
  Pinout.pdf`, in `adafruit/Adafruit-MacroPad-RP2040-PCB` on GitHub)
  accounts for every externally-reachable connection on the board: the
  4-pin STEMMA QT connector (3.3V / GND / SDA=GPIO20 / SCL=GPIO21) and the
  onboard peripherals already wired to specific pins (keys on GPIO1–12 —
  confirming `keypad_input.h`'s existing placeholder was actually correct
  — rotary on 17/18, OLED on 22–24/26/27/28, onboard NeoPixel on 19,
  speaker on 14/16, boot button on 0, status LED on 13).
- GPIO15, GPIO25, and GPIO29 exist on the RP2040 silicon but have **no
  pad, test point, or header anywhere on this board**. They are not "hard
  to find" — they are not physically exposed at all, confirmed by
  Adafruit's own schematic/pinout source, not just the learn-guide text.

**Conclusion:** the MacroPad cannot gain any new general-purpose output
without either board-level rework (rejected, see below) or sacrificing an
onboard peripheral (e.g. the speaker — also rejected, since its signal
passes through a Class-D amp + RC filter that would likely distort WS2812
timing even if the pins were reclaimed).

### Companion controller: Adafruit QT Py RP2040 over STEMMA QT

The MacroPad's only real expansion point is its STEMMA QT I2C bus — already
the intended home for the (now-removed) MCP23017. A second, small RP2040
board fits that same role and solves the GPIO problem outright:

- **Powering it:** verified against the QT Py RP2040's own Eagle schematic
  (`adafruit/Adafruit-QT-Py-RP2040-PCB`) — the STEMMA QT connector's `V+`
  pin is on the same `+3V3` net as the RP2040 chip's own `VCC` pin (not a
  separate downstream-only supply). This board is designed to be
  back-powered over STEMMA QT with no USB connection, which is exactly how
  it'll be run here: powered from the MacroPad's own STEMMA QT cable.
  **Unverified / left to the builder:** confirm the MacroPad's 3.3V
  regulator has enough spare headroom once both boards' real current draw
  is measured — expected to be fine, since the QT Py only drives
  logic-level data lines, not the strips' own power, but not yet measured.
- **Why it beats the MCP23017's role and the NeoDriver alternative:** the
  QT Py has 264KB of RAM (a 540-pixel buffer needs ~1.6KB) and native
  RP2040 PIO, so it can generate both strips' WS2812 data directly with no
  practical pixel-count ceiling.
- **NeoDriver (Adafruit I2C-to-NeoPixel driver) was evaluated and
  rejected:** per its own learn guide, it's built on an ATtiny1616 running
  "seesaw" firmware with a **hard 512-pixel RAM ceiling** — not
  configurable. The 18m/540-LED strip exceeds that by 28 pixels. Covering
  it would mean physically cutting the strip in two and driving each half
  from a separate NeoDriver unit, which (since the Diff-Adv Receiver only
  decodes one of its four incoming channels per unit) would also require a
  *second* receiver for that one strip — more hardware, more cost, and it
  defeats the actual point of this test (confirming one continuous 18m run
  works end to end, not two independently-driven half-runs).
- **GPIO assignment on the QT Py:** two free, PIO-capable GPIO pins, one
  per strip — exact pin numbers are an implementation detail for the
  builder (the QT Py breaks out 11 GPIO, `D0`–`D10`; any two work), not a
  design decision that needs to be pinned down here.

### Signal and power chain (decided)

**Confirmed: the MacroPad, QT Py, and Diff-Adv Sender-4 are all co-located**
as one cluster (consistent with the QT Py's GPIO-to-Sender connection being
a short logic-level wire, not something meant to span a long run). The long
differential run happens entirely on the Sender→Receiver CAT cable.

**Confirmed: the two Receiver Midpoint-1 units are daisy-chained** on one
CAT run from the Sender — the first receiver decodes its one selected
channel and passes the other three through its RJ45 pass-through to the
second receiver downstream, rather than each getting its own home-run cable
back to the Sender. Physical order (which strip's receiver sits closer to
the Sender) is a cable-routing detail at install time, not a design
decision — with only two receivers and a 500m-rated run, leftover cable
budget isn't a practical constraint either way.

```
[MacroPad RP2040] --STEMMA QT I2C (SDA=GPIO20/SCL=GPIO21)--> [QT Py RP2040]
                                                                |        |
                                                 PIO-driven WS2812 data  |
                                                        (one GPIO each)  |
                                                                v        v
                                                   [Diff-Adv Sender-4 input ch.1]
                                                   [Diff-Adv Sender-4 input ch.2]
                              (MacroPad + QT Py + Sender-4 all co-located)
                                                                |
                                          (both channels multiplexed onto)
                                                                v
                                         1x differential RS-485 signal,
                                         over CAT cable (spec'd to 500m)
                                                                |
                                                                v
                                              [Receiver Midpoint-1 #1]
                                              DIP switch selects its channel
                                              local 5V power in (buck from
                                              the 24V main supply)
                                              WS2812 data+power out  |
                                                   |                |
                                      18m or 12m strip     RJ45 pass-through
                                      (whichever channel          |
                                       this unit decodes)         v
                                                        [Receiver Midpoint-1 #2]
                                                        DIP switch selects the
                                                        other channel
                                                        local 5V power in (buck
                                                        from the 24V main supply)
                                                        WS2812 data+power out
                                                                |
                                                     the other strip (18m or 12m)
```

Key facts behind this chain, from the QuinLED spec pages:

- **Diff-Adv Sender-4**: 4 independent LED-data input channels (3.3V/5V
  logic), combined onto 1 differential RS-485 output via RJ45, rated to
  500m over UTP cable. Its own 24V input only powers its internal signal
  electronics (~150mA at 24V) — it does **not** carry power to the
  receiver.
- **Diff-Adv Receiver Midpoint-1**: decodes 1 of 4 incoming channels
  (selected via onboard DIP switch), passes the other 3 through
  downstream. Needs its own local power supply wired directly to its
  screw terminals — confirmed via the product's own text: *"You only need
  a power supply of the same voltage as your LEDs and the receiver board
  and you are done at that location!"* Output is fused at **5A standard
  (10A max if the fuse is swapped)**, at whatever voltage feeds its local
  VCC input.
- **Final installation power plan** (not this doc's test-mode scope):
  **confirmed** — one 24V/75A/1800W supply, with 10AWG cable run directly
  (home-run style, not daisy-chained) from that supply to the Sender, to
  each of the two Receivers, and to further buck converters at additional
  mid-strip power-injection points as needed. Power distribution is
  therefore entirely separate from the signal chain's daisy-chain — the
  CAT cable only ever carries data between the Sender and Receivers; every
  device's 24V feed comes directly off the main supply via its own 10AWG
  run. Each Receiver (and any other injection point) steps 24V down to 5V
  locally via its own buck converter, since WS2812B needs 5V and the main
  rail is 24V.

## Test-mode power budget (decided)

Goal: confirm signal and power integrity across the full length of each
strip using *only* the receiver's own local 5A/5V budget — no additional
mid-strip power injection yet.

- Fuse budget: 5A × 5V = **25W** per receiver.
- BTF-Lighting WS2812B ECO, 30 LEDs/m, rated "max 6W/m" at 5V → **1.2A/m**,
  or 0.2W/LED at full white (all 3 channels on).
- A single primary color (e.g. pure red, green or blue — only one channel
  on) draws roughly 1/3 of the full-white max: **~0.0667W ≈ 13.3mA/LED**.
- Lighting every **other** LED (the densest "skip" pattern that still
  shows the whole strip's length over one full odd/even cycle) at full
  single-color brightness:

  | Strip | Total LEDs | Lit (every-other) | Current | Power | % of 25W budget |
  |---|---|---|---|---|---|
  | 18m | 540 | 270 | 3.6A | 18W | 72% |
  | 12m | 360 | 180 | 2.4A | 12W | 48% |

  Both stay under the conventional 80%-of-fuse safety margin (4A/20W).
  Lighting *every* LED with no skipping would need brightness scaled to
  roughly 55–70% to stay safe on the 18m strip, which defeats the
  "full-brightness, confirm full reach" intent — every-other is the
  practical ceiling without dimming.

## Test Pattern UI (decided)

Added as a 4th top-level OLED menu item, after the existing
`Brightness → Presets → Custom` list (`ui_controller.h`'s `_uiTopHighlight
% 3` becomes `% 4`):

- **OLED-menu-only, no keypad verb code.** This is a diagnostic tool, not
  a production preset — it doesn't need the full verb/noun dual-access
  treatment the nav design doc's "dual accessibility" principle otherwise
  requires. This is a deliberate, documented exception, the same way
  Brightness is an intentional encoder-only exception for a different
  reason.
- **No preview/confirm two-step.** There's nothing to dial in — selecting
  "Test Pattern" and pressing the rotary starts the cycle immediately.
- **Sequence** (5 seconds per phase, 30-second full cycle), applied in
  lockstep to **both outdoor strips and the MacroPad's own 12 onboard
  keypad NeoPixels** (6 "odd"-index keys / 6 "even"-index keys) so all
  three LED surfaces are visibly in sync:

  1. RED, odd LEDs — 5s
  2. RED, even LEDs — 5s
  3. GREEN, odd LEDs — 5s
  4. GREEN, even LEDs — 5s
  5. BLUE, odd LEDs — 5s
  6. BLUE, even LEDs — 5s
  7. repeat from 1

- **Named constants** (not hardcoded), so both are easy to retune later:
  `LED_TEST_PATTERN_STRIDE` (2, for every-other) and
  `LED_TEST_PATTERN_PHASE_MS` (5000).
- **Always renders at fixed, full single-color brightness — the global
  Brightness setting is ignored while Test Pattern is active.** The power
  budget table above is computed at full per-LED current draw; if the
  existing brightness scalar were applied here too, the test would
  sometimes run dimmer than analyzed, which would make it falsely pass
  (looks fine at reduced power) without ever actually validating the
  worst-case draw it's meant to prove. Builder's call, not a safety risk
  either way (scaling down only ever reduces draw further below the
  budget), but worth being deliberate about so the test stays meaningful.
- **Not persisted.** Running Test Pattern never calls `saveSettings()` and
  never changes the confirmed active scheme. Exiting (pressing `<verb>` or
  the rotary again) returns to the top menu, and whichever scheme was
  previously confirmed simply resumes — matching this project's existing
  "nothing physical changes until confirm" philosophy, just inverted (here,
  *leaving* test mode is what restores the real state).

## Repo structure (decided)

A monorepo holding both microcontrollers' firmware plus one shared local
library, since Arduino's tooling requires one sketch = one folder = one
`.ino` matching the folder name, and a sketch targets exactly one board:

```
arduino-macropad-led-controller/          (repo root — unchanged in spirit)
├── arduino-macropad-led-controller.ino   (existing MacroPad sketch)
├── sketch.yaml                           (existing — MacroPad FQBN/libs)
├── debug.h, colors.h, gpio_expander.h, ...  (existing headers — gpio_expander.h
│                                              becomes dead code once MCP23017
│                                              is fully removed; see "left to
│                                              the builder")
├── qtpy_led_driver/
│   ├── qtpy_led_driver.ino               (new — QT Py sketch)
│   └── sketch.yaml                       (new — QT Py FQBN/libs, separate
│                                              from the MacroPad's)
└── MacropadShared/
    ├── library.properties                (new — makes this a proper
    │                                           Arduino library)
    └── src/
        └── led_test_pattern.h            (new — hardware-agnostic
                                                 color/parity/phase sequencing,
                                                 shared by both sketches)
```

`MacropadShared` holds only genuinely hardware-agnostic logic — "what color
and odd/even phase should be active right now, given elapsed time" is
identical math whether it's driving 12 keypad pixels or 540 strip pixels.
Each sketch keeps its own hardware-specific code (which `Adafruit_NeoPixel`
instance, which pins, how many LEDs) and just calls into the shared logic
for the sequencing decision.

**Unverified / left to the builder:** the exact mechanics of how
`arduino-cli`/`sketch.yaml` resolve a local shared library across two
sibling sketches in the same repo (vs. one installed globally in the
Arduino sketchbook folder) haven't been tested. Verify this hands-on early
— before writing much logic into `MacropadShared` — in the same spirit as
this project's existing "verify library calls before trusting them"
practice. If local-library resolution turns out not to work cleanly, the
fallback is duplicating the (small) shared logic into each sketch rather
than blocking the whole feature on tooling.

## MacroPad ↔ QT Py command link (decided — minimal, Test-Pattern-scoped)

Scope deliberately kept narrow: this covers only what Test Pattern needs,
not a general preset-rendering protocol (that's future work, pending the
still-undecided monthly scheme content). The QT Py acts as an I2C
peripheral; the MacroPad is the I2C host (same bus role the MCP23017 used
to occupy). Two commands are enough for this feature:

- **Start Test Pattern** — QT Py begins the RED/GREEN/BLUE ×
  odd/even × 5s cycle on both its strips, free-running off its own clock
  (it does not need the MacroPad to re-send anything during the cycle).
- **Stop Test Pattern** — QT Py turns both strips off and returns to idle.

Exact command byte values/framing are an implementation detail for the
builder — a single command byte with no payload is sufficient for just
these two cases.

**Added: a lightweight ACK on the Start command** (the MacroPad does an I2C
read immediately after writing Start, expecting a fixed response byte).
Without this, if the QT Py is unpowered, miswired, or the I2C transaction
fails, the OLED would show "Test Pattern running" with nothing actually
happening and no way to tell why — which undermines a feature whose whole
purpose is validating that the hardware link works. This is cheap (one
extra I2C transaction, only on Start) and doesn't expand the protocol's
scope the way a full status-reporting link would. Stop stays fire-and-forget.

**Clock drift between the two boards' free-running cycles:** not a
concern at these timescales — RP2040 crystal tolerances drift by
microseconds over a single 5s phase, far below what's visible to a person
watching the strips. No resync mechanism needed.

## Rejected / superseded alternatives (for reference)

- **MCP23017-driven GPIO for the strips** — the original plan; superseded
  once WS2812B was chosen (see "Why the MCP23017 can't drive WS2812B").
- **Direct MacroPad RP2040 GPIO (e.g. GPIO15/GPIO25)** — looked viable
  until the official pinout/schematic check showed those pins aren't
  physically exposed anywhere on the board.
- **Reclaiming the onboard speaker's GPIO14/16** — technically wired and
  accessible, but the signal path runs through a Class-D amp + RC filter
  that would likely distort WS2812's tight edge timing; not pursued.
- **Direct board-level rework** (bodge wires to the RP2040 package) — not
  realistic for this build, not pursued.
- **2x Adafruit NeoDriver, strip physically cut in half** — covers the
  pixel count but requires a second receiver and splits the one continuous
  18m run this test is specifically meant to validate; rejected in favor
  of the QT Py.

## Open questions / left to the builder

- Measure real combined current draw (MacroPad + QT Py) against the
  MacroPad's 3.3V regulator headroom before relying on STEMMA QT
  back-powering in the field.
- Verify `arduino-cli`/`sketch.yaml` local shared-library resolution
  across the two sketches (see "Repo structure").
- Exact QT Py GPIO pin numbers for the two strip data lines (any two of
  its free, PIO-capable pins).
- Exact I2C command byte values for Start/Stop Test Pattern.
- `gpio_expander.h` and the MCP23017 library dependency in `sketch.yaml`
  become dead code once this design is implemented — remove them as part
  of implementation rather than leaving an unused `HARDWARE_GPIO_EXPANDER`
  path around.
- CLAUDE.md needs updating once this doc is approved: the "Open design
  questions" entry on LED type is now resolved, the hardware architecture
  section needs the QT Py + QuinLED chain added, and the MCP23017 section
  needs removing — tracked here so it isn't forgotten, but treated as
  implementation work, not part of this design doc itself.
