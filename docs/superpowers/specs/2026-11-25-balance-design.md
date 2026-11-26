# Balance setting — design

## Intent

A global **Balance** setting that shifts brightness between the top strip
(A1/12m/2nd floor) and the bottom strip (A0/18m/1st floor) without ever
raising either above the Brightness setting. Besides aesthetic balancing,
the extremes double as a maintenance switch: Balance 0% turns the top
strip fully off (its floor is hard to reach for repairs), 100% turns the
bottom strip off.

## Behavior (user-decided)

- New top-menu item **"Balance"**, directly below Brightness. Rotary-only,
  same interaction as Brightness: knob adjusts, press confirms, live
  preview on the real strips while turning, persisted only on confirm.
- OLED shows the same style of bar as Brightness. **50% = even** (the
  default).
- Turning right (above 50%) dims the **bottom** strip; turning left
  (below 50%) dims the **top** strip. Never brightens either.
- **Fully dark at the ends**: 0% → top strip off, 100% → bottom strip off.
- Keypad preview reflects balance by dimming the top 6 keys (top strip) or
  bottom 6 keys (bottom strip).
- Global setting, saved on confirm, restored on boot.

## Approaches

- **A — MacroPad computes per-strip brightness (chosen).** MacroPad folds
  Brightness × Balance into a top % and a bottom % and sends those; the QT
  Py just applies one percent per strip and has no notion of "balance".
  Keeps the MacroPad = instructor / QT Py = executor split, so future
  changes to the balance curve are MacroPad-only reflashes.
- B — send Balance as its own field and let the QT Py combine it. Puts
  logic on the QT Py; curve changes would need QT Py reflashes.
- C — bake balance into the palette colors. Live preview still needs a
  new command, and the Query response would no longer report true preset
  colors. Rejected.

## Design

- **Math** (shared, `MacropadShared`, unit-tested): balance `b` 0-100,
  linear on each side of center —
  `topScale = b < 50 ? b*2 : 100`, `bottomScale = b > 50 ? (100-b)*2 : 100`;
  each strip's percent = `brightness * scale / 100`. 5% per detent (same
  as Brightness), so 10 detents from center to a dark strip.
- **Protocol** (I2C change — both boards flash together):
  - `SchemeRecipe.brightness` is replaced by `topBrightness` and
    `bottomBrightness` (packed size 52 → 53).
  - `QTPY_CMD_PREVIEW_BRIGHTNESS` carries 2 bytes (top %, bottom %) and
    serves the live preview for both the Brightness and Balance knobs.
- **QT Py**: renders each strip at its own percent. Flash layout gets a
  new magic; older saves migrate (old single brightness → both strips).
  Because per-strip percents persist on the QT Py, a dark top floor stays
  dark through a power cut even without the MacroPad.
- **MacroPad**: settings storage gains a balance byte (new magic; older
  saves migrate with balance = 50). Top menu becomes Brightness, Balance,
  Presets, Custom, Test Pattern.
- **Keypad previews**: the Balance screen previews the active scheme at
  the pending balance. The Presets and Brightness previews also apply the
  confirmed balance, so every preview matches the strips.

## Builder's calls (flagged for review)

- OLED Balance screen adds a third row showing the result, e.g.
  `Top 100%  Bot  40%` (balance-only scales), so the maintenance case
  reads unambiguously.
- Test Pattern ignores Balance — a diagnostic must light every LED.
- Balance has no keypad (verb/noun) path, same as Brightness (continuous
  controls are encoder-only).
