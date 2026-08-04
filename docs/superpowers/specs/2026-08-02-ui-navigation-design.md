# UI / Navigation Design — MacroPad LED Controller

**Status:** DRAFT — in progress, being built collaboratively section by section.
This file is being updated live during brainstorming so the plan can be picked
back up if the conversation is interrupted. Treat anything below as tentative
until the "Status" line at the top says APPROVED.

## Keypad layout

Physical 3x4 grid, laid out like a 10-key numpad with the bottom-right two
keys repurposed:

```
<1> <2> <3>
<4> <5> <6>
<7> <8> <9>
<0> <verb> <noun>
```

- Keys 1–9 and 0 (10 keys) map to numeric/ordinal input.
- `<verb>` and `<noun>` (bottom row, positions 11 and 12) are not numeric —
  their exact behavior is still being defined (next topic in discussion).

`<verb>` code 1 is reserved as a meta-verb, **"Preset"** — this is what
unifies monthly-preset selection and custom-design building into one
consistent flow (see below), resolving the earlier "12 presets vs 10 keys"
tension: preset choice moves into the noun-digit field as a 1-or-2-digit
month number (1-12), not a dedicated key.

## Verb / Noun concept

Inspired by the Apollo Guidance Computer's DSKY (VERB/NOUN keypad interface).

- `<verb>` = choose an action: "Preset" (verb 1 — recall one of the 12
  built-in monthly presets, read-only/never overwritten), "Custom" (verb 2 —
  recall the one saved custom design), or an animation/pattern to build a new
  custom design (solid, blink, chase, fade, rainbow... verb 3+, exact list
  TBD).
- `<noun>` = choose the parameter for that verb. Meaning is **contextual**:
  when verb = Preset, the noun digits are the month number (1-12); when
  building a custom design (verb 3+), the noun digits pick a color from a
  palette; verb = Custom (recall) takes no noun.
- Nothing takes effect on the real outdoor installation until explicitly
  confirmed (see "Preview vs. commit" below) — this replaced an earlier,
  simpler idea of instant-apply-per-digit once the confirmation requirement
  came up.
- Goal: simple by default, with optional depth (encoder fine-tuning) for
  users who want to customize further.

## Global principles (decided)

- **Dual accessibility**: every action must be reachable two ways — via the
  keypad (verb/noun/digit sequence below) *and* via an OLED menu navigated
  with the rotary encoder alone. Neither path is a second-class citizen.
  (The OLED-menu-only path's exact design is still open — see Open
  questions.)
- **Preview vs. commit**: the 12 onboard MacroPad NeoPixels (indicator LEDs
  under the keys) are used as a **live preview** of the pending
  selection/edit — they update immediately on every keystroke/encoder turn.
  The actual outdoor installation LEDs are only updated when the selection
  is **confirmed** (rotary encoder press). Nothing about the physical
  installation changes until confirm.
- **Confirm = rotary press, does double duty**: pressing the encoder both
  commits the pending selection to the outdoor LEDs *and* saves it to flash
  in the same action — there isn't a separate "apply" vs. "save" gesture.
  After confirming, the onboard NeoPixels return to off/idle.

## Verb / Noun entry flow (decided — worked example)

Variable-length digit entry (not a fixed 1 or 2 digits), terminated by
pressing the *next* field's key (`<verb>` → digits → `<noun>` ends verb
entry) or, at the end, by the rotary press (ends noun entry and confirms).
OLED updates on every keystroke; onboard NeoPixels preview live.

Worked example — recalling the December preset:

| Step | Key pressed | OLED shows | Onboard NeoPixels |
|------|-------------|-----------|--------------------|
| 1 | `<verb>` | "Verb" (awaiting verb code) | — |
| 2 | `1` | "Preset" (verb 1 resolved) | — |
| 3 | `<noun>` | (locks in verb=Preset, awaiting noun code) | — |
| 4 | `1` | "1" | preview of January (preset 1) |
| 5 | `2` | "12" (digit appended) | preview switches to December (preset 12) |
| 6 | — | "Confirm: Rotary Press" | still previewing December |
| 7 | *(rotary press)* | selection confirmed | outdoor LEDs update to December; onboard LEDs turn off; saved to flash |

Building a new custom pattern+color follows the same shape, just with a
pattern verb code (3+, e.g. "Chase") at step 2 and the noun digits at steps
4-5 resolving to a color palette entry instead of a month. Recalling the
existing saved custom design is shorter still: `<verb>` 2 ("Custom") needs no
noun at all — straight to "Confirm: Rotary Press" after the verb resolves.

**Encoder rotation** (as opposed to the press, which confirms) fine-tunes
whichever field was most recently entered:
- After noun digits resolve to a color, rotating sweeps **hue** continuously
  from that palette color.
- After verb digits resolve to a (non-Preset) pattern, rotating adjusts
  **animation speed**.
- Resolution rule: "last thing you picked, the knob adjusts it."

### Rotary detent behavior (decided)

Per Adafruit's own Arduino example for this board
([learn.adafruit.com/adafruit-macropad-rp2040/arduino](https://learn.adafruit.com/adafruit-macropad-rp2040/arduino)),
which configures the third-party `RotaryEncoder` library with
`RotaryEncoder::LatchMode::FOUR3` — this rotary encoder's detents don't line
up 1:1 with a single quadrature-decoded step; it takes 2 quadrature-decoded
steps per felt detent. `rotary_input.h` relies on the `RotaryEncoder`
library itself (rather than a hand-rolled quadrature decoder) with that same
`LatchMode::FOUR3` configuration, so this is handled internally by the
library — `pollRotaryDelta()` (`rotary_input.h`) reports one ±1 per felt
detent, and every consumer (menu scroll, hue/speed fine-tune) just sees that,
without needing its own accumulate-by-2 workaround.

The "Hue fine-tune math" below was written assuming 1 felt detent = 1
"rotary step" of hue-wheel spacing, which still holds — `pollRotaryDelta()`
reports in felt detents either way, so the "10 steps apart / 36° apart / 5
rotations around the wheel" conclusion is unaffected by this detail.

### Hue fine-tune math (decided)

Hardware fact (already noted in CLAUDE.md, repeated here since it drives this
math directly): the MacroPad's rotary encoder has **20 detents per 360°
physical rotation**.

- The 10 palette colors (one per digit, 0-9 — see "Color depth" under Verb /
  Noun concept) are spaced **10 encoder steps apart** on the hue wheel — a
  half physical turn moves you exactly from one named color to the next.
- Since there are 10 colors and 10 steps between each, they sit **36° apart**
  around the full 360° hue circle (360°/10), and a complete trip around the
  entire hue wheel takes **5 full physical rotations** of the knob (10 colors
  × 10 steps ÷ 20 steps/rev). "Steps" here means felt detents, per the
  "Encoder detent behavior" note above.
- Turning the encoder ±10 steps from a picked base color reaches exactly to
  the adjacent color's own hue — full, gapless, non-overlapping-beyond-that
  coverage of the entire hue spectrum from any starting point.
- **Caveat for the pattern/color content session**: this clean math only
  holds for colors that are pure hues on the wheel. Something like **white**
  has no hue (zero saturation) and can't occupy one of these 10 evenly-spaced
  positions the same way — worth remembering when the actual 10-color list
  gets picked.

Rejected/superseded alternatives (for reference): a fixed single-digit
instant-apply model with no confirmation step (superseded once the
confirm/preview requirement came up); a fixed two-digit DSKY code (replaced
by variable-length, terminated-by-next-key entry); a dedicated preset key
range separate from verb/noun (replaced by verb 1 = "Preset").

**Cancel/restart (decided)**: there's no dedicated cancel key — pressing
`<verb>` again at any point during an in-progress sequence discards whatever
was entered and restarts verb entry from scratch ("Verb" prompt). Nothing
pending was ever committed to the outdoor LEDs, so restarting is always
safe. **Refinement**: if `<verb>` is pressed while *already* sitting at that
freshly-restarted "waiting for a verb digit" state (i.e. a second `<verb>`
press with nothing typed in between), it goes one step further and clears
the input line back to idle entirely, rather than restarting into the exact
same state it was already in — a double-tap-`<verb>` full cancel.

**Bare digit press with no active sequence (decided)**: ignored as an
action — `<verb>` is always the required entry point, no fast-path
shortcuts. However, pressing a bare digit while idle should trigger a brief
OLED hint (e.g. "Press Verb to start") so it's discoverable rather than
silently doing nothing.

## Persistence (decided — implemented for scheme + brightness)

- Confirming (rotary press) both applies to the outdoor LEDs **and** saves
  to flash in one action (see "Global principles" above) — no separate
  save gesture.
- **Implemented**: the active preset (scheme index) and global brightness
  are saved together to flash on every confirm (Presets or Brightness — see
  `settings_storage.h`, via the arduino-pico core's built-in EEPROM
  emulation) and restored on boot, satisfying the "Boot behavior" point
  below for these two settings. Still stubbed: actually applying the
  restored brightness to the *outdoor* installation, since `led_output.h`
  has no real API yet — only the UI's own state (`getActiveScheme()`,
  the confirmed brightness) is restored so far.
- **Custom vs. presets (decided)**: the 12 monthly presets are fixed/built-in
  and never overwritten. Every custom pattern+color build saves into one
  dedicated "Custom" slot (recalled via `<verb>` 2 — see "Verb / Noun
  concept" above), not into any month. This keeps the 12-month collection
  stable year to year while still allowing one persistent custom look.
  (Not yet implemented in code — "Custom" itself is still a stub.)
- **Boot behavior**: the controller remembers the last-confirmed selection
  (whichever preset, or the custom slot, was active) and restores it after a
  power cycle — an outdoor installation shouldn't revert to a fixed default
  (e.g. January) after a storm-related power blip. Implemented for
  preset+brightness as above; the custom slot isn't built yet.

## Encoder-only / OLED menu path (decided — shape)

- One flat, scrollable top-level list on the OLED, populated with the same
  verbs as the keypad flow: "Preset", then each pattern ("Chase", "Blink",
  "Fade", ...). Turning the encoder scrolls this list; pressing descends.
- Descending into "Preset" shows a scrollable list of the 12 months
  (Jan-Dec); descending into any other verb shows the color palette list.
  Turning scrolls with live preview on the onboard NeoPixels (same
  preview-vs-commit rule as the keypad path); pressing confirms — applies to
  the outdoor LEDs and saves to flash, same as the keypad flow's rotary
  press.
- This mirrors the keypad flow's shape (verb, then noun, then confirm) —
  turn-and-press substitutes for digit entry, but the underlying states and
  confirm semantics are shared between both input paths.
- Not yet addressed: exact wording of menu entries on a 128x64 OLED,
  scroll-position indicators, and whether hue/speed fine-tuning (available
  post-confirm... or post-noun-selection?) is reachable in this path the
  same way it is via digit entry — likely yes, but not explicitly walked
  through yet.

## OLED font convention (decided)

Per Adafruit's own Arduino example for this board
([learn.adafruit.com/adafruit-macropad-rp2040/arduino](https://learn.adafruit.com/adafruit-macropad-rp2040/arduino)):

- `display.setTextSize(1);` — this is the size to use (already applied
  throughout `oled_display.h`'s rendering functions as of the first coding
  pass — flagged here per our convention-pushback agreement, now resolved).
- `display.setTextWrap(false);` — no automatic line wrapping.
- `display.setTextColor(SH110X_WHITE, SH110X_BLACK);` — explicit background
  color (not just foreground), so redrawing text at the same position
  overwrites cleanly without needing a full `clearDisplay()` first. Useful
  for this design's frequent live-updating text (digit entry, menu
  scrolling) without visible flicker.

## Preset preview swatch content — saturation lesson (decided)

The 12 monthly preset-preview colors live in code
(`led_presets.h`'s `getSchemePreviewColors()`), not spelled out in this doc —
but one lesson from building them is worth recording here since it'll apply
to any future color content (the eventual Custom-build palette included):

- April's original "pastel" values (e.g. pastel pink at `Color(255, 182,
  193)`) sat at only **~25-32% saturation** and read as almost
  indistinguishable from white on the actual keypad NeoPixels.
- Bumped to **~50-59% saturation** (e.g. that same pink became
  `Color(220, 90, 140)`) — still visibly softer/lighter than the
  fully-saturated primaries used elsewhere (`PIXEL_RED` etc. sit at 100%
  saturation), but now clearly tinted rather than washed out.
- Takeaway: on this hardware, "pastel" shouldn't be interpreted as
  low-saturation-near-white — aim for roughly half-saturated rather than
  quarter-saturated when a color needs to read as soft *and* identifiable.

## Brightness menu (decided)

Added as a new top-level menu item, **above** Presets: the top-level list is
now "Brightness", "Presets", "Custom" (in that order).

- Entering it previews the *currently active* scheme (whichever preset — or
  eventually Custom design — is presently confirmed, not whatever's pending
  in an unconfirmed Presets browse) on the keypad LEDs, scaled to the
  current brightness level, plus a text progress-bar readout on the OLED
  (`[#########     ] 68%`).
- Turning the encoder adjusts brightness live (0-100%, in 5%
  per-felt-detent steps — a full rotation covers the whole 0-100% range),
  updating both the bar graph and the keypad-LED preview brightness in
  real time.
- Rotary press confirms — same "Rotary Press: Confirm" messaging convention
  as Presets. On confirm: the new brightness becomes the active level
  (applied to every future keypad-LED preview, not just this session), gets
  applied to the outdoor installation, and is saved to flash — mirroring
  the Presets confirm gesture exactly.
- **Encoder-only, by design (decided)**: unlike Preset/Custom, Brightness
  does not get a keypad verb/noun equivalent — a continuous 0-100 analog
  value is a more intuitive fit for the encoder's turn-to-adjust motion than
  digit entry, so this is an intentional, permanent exception to the
  "everything reachable two ways" principle, not a gap to close later.
- Flash persistence is implemented (see "Persistence" above,
  `settings_storage.h`) — only applying it to the *outdoor* installation is
  still stubbed, since no `led_output.h` API exists yet to actually call.

## Open questions

- Does any pattern (verb) ignore the noun color choice because it's
  inherently multi-color (e.g. rainbow)? Revisit once the actual pattern
  list is designed.
- The actual list of patterns (verbs) and color palette (nouns) themselves —
  content, not just mechanism (including reconciling the "white has no hue"
  caveat noted in the hue fine-tune math above). Likely its own follow-up
  design session.
- OLED menu path details: exact wording of entries on a 128x64 display,
  scroll-position indicators, and whether hue/speed fine-tuning is reachable
  the same way it is via digit entry (likely yes, not yet walked through).
